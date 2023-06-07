#include "include/TimerQueue.h"
#include "include/EventLoop.h"
#include "include/TimeDefs.h"
#include "include/TimerId.h"
#include "include/Timer.h"
#include "include/Channel.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iterator>
#include <memory>
#include <string>
#include <unistd.h>
#include <sys/timerfd.h>
#include <cassert>

#include <sstream>
#include <iostream>
#include <utility>
#include <vector>
#include <iomanip>

namespace Cloo::detail 
{

std::string SytemTimePointToStr(Cloo::define::SystemTimePoint time_point)
{
  std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%F %T") << "." << std::setfill('0') << std::setw(3) << ms.count();
  return ss.str();
}

// 对timerfd_create系统调用的简单封装
// 作用是创建一个定时器, 定时器到期事件由timerfd通知
int CreateTimerFd()
{
    int timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timer_fd < 0)
    {
        std::cerr << "Failed in timerfd_create";
        abort();
    }
    return timer_fd;
}

timespec HowMuchTimeFromNow(Cloo::define::SystemTimePoint when)
{
    auto duration = when - std::chrono::system_clock::now();
    timespec ts {0} ;
    ts.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()%1000000000;
    return ts;
}

void ReadTimerFd(int timerfd, Cloo::define::SystemTimePoint now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    std::cout << "TimerQueue read " << howmany << " at " << SytemTimePointToStr(now) << std::endl;
    if(n != sizeof howmany)
    {
        std::cerr << "TimerQueue reads " << n <<" bytes instead of " << sizeof howmany << std::endl;
    }
}

void ResetTimerFd(int timerfd, std::chrono::time_point<std::chrono::system_clock> expiration)
{
    itimerspec new_value{0}, old_value{0};
    auto time_from_now = expiration - std::chrono::system_clock::now();
    new_value.it_value = HowMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
    if(ret != 0)
    {
        std::cerr << "timer_settime()";
    }
}

}

using namespace Cloo;
using namespace Cloo::detail;
using namespace std;

TimerQueue::TimerQueue(const shared_ptr<EventLoop>& loop)
    : owner_loop_(loop),
      timer_fd_(CreateTimerFd()),
      timer_fd_channel_(Channel::Create(loop, timer_fd_)),
      timers_(EntryComp) /*自定义的比较函数*/
{
    // 注册timerfd到期事件的回调
    timer_fd_channel_->SetReadCallBack(bind(&TimerQueue::HandleRead, this));
    // 将timerfd注册到EventLoop中
    timer_fd_channel_->EnableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timer_fd_);
}

TimerId TimerQueue::AddTimer(const define::TimerCallback &cb, define::SystemTimePoint when, long interval_ms)
{
    auto timer = make_shared<Timer>(cb, when, interval_ms);
    if(auto loop = owner_loop_.lock())
    {
        loop->RunTaskInThisLoop(bind(&TimerQueue::AddTimerInLoop,this,timer));
    }
    return TimerId(timer);
}

void TimerQueue::AddTimerInLoop(const shared_ptr<Timer>& timer)
{
    if(auto loop = owner_loop_.lock())
    {
        loop->AssertInLoopTread();
    }
    bool is_earlist = Insert(timer);
    if(is_earlist)
    {
        ResetTimerFd(timer_fd_, timer->Expiration());
    }
}

void TimerQueue::Cancel(const TimerId& timer_id)
{
    // do nothing
}

void TimerQueue::HandleRead()
{   
    auto loop = owner_loop_.lock(); 
    if(!loop)
    {
        cerr << "TimerQueue owner_loop does not exisit" << endl;
        abort();
    }
    loop->AssertInLoopTread();
    // 将timerfd的内容读出, 避免 "level-trigger" IO多路复用组件持续触发“可读”条件
    define::SystemTimePoint now = std::chrono::system_clock::now();
    ReadTimerFd(timer_fd_, now);
    // 从TimerList中找到目前为止所有的到期timer, 过期的timer的所有权从timers_转移到vector<Entry>中
    vector<Entry> expired_vec = GetExpiration(now);

    // 触发所有定时回调
    for_each(expired_vec.begin(), expired_vec.end(), [](const Entry& item){ item.second->Run(); });
    
    Reset(expired_vec, now);
    
}


vector<TimerQueue::Entry> TimerQueue::GetExpiration(define::SystemTimePoint now)
{
    vector<Entry> expired;
    // shared_ptr会去释放UINTPTR_MAX指向的地址, 导致非法访问
    // Entry sentry = make_pair(now, shared_ptr<Timer>(reinterpret_cast<Timer*>(UINTPTR_MAX)));
    // auto last_iter = timers_.lower_bound(sentry);
    decltype(timers_.begin()) last_iter;
    for(auto iter = timers_.begin(); iter != timers_.end(); iter++)
    {
        if(iter->first > now)
        {
            last_iter = iter;
            break;
        }
    }
    // FIXME: 这里为什么需要assert?
    assert(last_iter == timers_.end() || now < last_iter->first);

    copy(timers_.begin(), last_iter, back_inserter(expired));

    timers_.erase(timers_.begin(), last_iter);

    return expired;
}

void TimerQueue::Reset(vector<Entry>& expired_vec, define::SystemTimePoint now)
{
    for_each(expired_vec.begin(), expired_vec.end(), 
        [&](Entry& entry)
        {
            if(entry.second->Repeat())
            {
                entry.second->Restart(now);
                Insert(entry.second);
            }
        });
    if(!timers_.empty())
    {
        auto next_expire = timers_.begin()->second->Expiration();
        if(next_expire > std::chrono::system_clock::now())
        {
            ResetTimerFd(timer_fd_, next_expire);
        }
    }
}

bool TimerQueue::Insert( const shared_ptr<Timer>& timer)
{
    bool earliest_expired = false;
    define::SystemTimePoint when = timer->Expiration();
    
    if(auto iter = timers_.begin(); iter == timers_.end() || when < iter->first)
    {
        earliest_expired = true;
    }
    auto result = timers_.insert(make_pair(when, timer));
    assert(result.second);

    return earliest_expired;
}