#include "include/EventLoop.h"
#include "include/CallbackDefs.h"
#include "include/Poller.h"
#include "include/Channel.h"
#include "include/TimerId.h"
#include "include/TimerQueue.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <memory>
#include <algorithm>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>
#include <sys/eventfd.h>

namespace Cloo::detail 
{

int CreateEventfd()
{
    int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(event_fd < 0)
    {
        std::cerr << "Fail to create eventfd.\n";
        abort();
    }
    return event_fd;
}

}

using namespace std;
using namespace Cloo;

// 当前线程所持有的EventLoop对象指针
thread_local shared_ptr<EventLoop> T_LOOP_IN_THIS_THREAD = nullptr;
// Poll超时时间(ms)EventLoopGetEventLoopOfThi
const int K_POLL_TIMEOUT_MS = 10000;

std::shared_ptr<EventLoop> EventLoop::Create()
{
    auto loop = shared_ptr<EventLoop>(new EventLoop());
    loop->looping_ = false;
    loop->quit_ = false;
    loop->handling_pending_tasks_ = false;
    loop->thread_id_ = this_thread::get_id();
    loop->poller_ = make_unique<Poller>(loop);
    loop->active_channels_ = make_shared<ChannelList>();
    loop->timer_queue_ = make_unique<TimerQueue>(loop);
    loop->wakeup_fd_ = (detail::CreateEventfd()),
    loop->wakeup_channel_ = Channel::Create(loop, loop->wakeup_fd_);

    cout<<"EventLoop created " << loop.get() << " in thread " << loop->thread_id_ <<endl;
    if(T_LOOP_IN_THIS_THREAD)
    {
        cerr<<"Another EventLoop " << T_LOOP_IN_THIS_THREAD << " exists in this thread " << loop->thread_id_ << endl;
        abort();
    }
    else
    {
        T_LOOP_IN_THIS_THREAD = loop;
    }
    return loop;
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    T_LOOP_IN_THIS_THREAD = nullptr;
}

void EventLoop::Loop()
{
    assert(!looping_);
    AssertInLoopTread();
    looping_ = true;
    quit_ = false;

    while(!quit_)
    {
        active_channels_->clear();
        // 通过poll(2)IO多路复用获取当前有活动事件的fd, 将活动事件通过channel转发过来
        poller_->Poll(K_POLL_TIMEOUT_MS, active_channels_);
        // 直接在IO线程中利用用户在channel中注册的callback function处理channel转发的IO事件
        std::for_each(active_channels_->begin(), active_channels_->end(), [](const auto& channel){channel->HandleEvent();});
        // 处理投放到pending_callbacks_中pending的事务
        DoPendingTasks();
    }
    cout << "EventLoop " << this << " stop looping" << endl;
    looping_ = false;
}

void EventLoop::UpdateChannel(const std::shared_ptr<Channel>& channel)
{
    assert(channel->OwnerLoop() == shared_from_this());
    AssertInLoopTread();
    poller_->UpdateChannel(channel);
}

shared_ptr<EventLoop> EventLoop::GetEventLoopOfThisThread()
{
    return T_LOOP_IN_THIS_THREAD;
}

void EventLoop::Quit()
{
    quit_ = true;
    // FIXME: 为什么其他线程调用Quit方法需要将EventLoop wakeup?
    if(!IsInLoopThread())
    {
        WakeUp();
    }
}

void EventLoop::RunTaskInThisLoop(const define::IOEventCallback &cb)
{
    if(IsInLoopThread())
    {
        cb();
    }
    else
    {
        QueueTaskInThisLoop(cb);
    }
}

void EventLoop::QueueTaskInThisLoop(const define::IOEventCallback &cb)
{
    // 缩小mutex的粒度
    if(1)
    {
        std::lock_guard<std::mutex> lg(mutex_);
        pending_tasks_.push_back(cb);
    }
    if(!IsInLoopThread() /*在其他线程*/ || handling_pending_tasks_ /*本线程中正在处理pendding callbacks*/)
    {
        WakeUp();        
    }
}

void EventLoop::DoPendingTasks()
{
    vector<define::IOEventCallback> callbacks;
    handling_pending_tasks_ = true;
    // 控制mutex的粒度
    if(1)
    {
        lock_guard<mutex> lg(mutex_);
        callbacks.swap(pending_tasks_);
    }
    for(const auto& cb : callbacks)
    {
        cb();
    }
    handling_pending_tasks_ = false;
}

void EventLoop::WakeUp()
{
    constexpr uint64_t one = 1;
    size_t n = ::write(wakeup_fd_, &one, sizeof one);
    if(n != sizeof one)
    {
        std::cerr << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::HandleWakeUp()
{
    uint64_t one = 1;
    auto n = ::read(wakeup_fd_, &one, sizeof one);
    if(n != sizeof one)
    {
        std::cerr << "EventLoop::HandleWakeUp() reads " << n << " bytes instead of 8";
    }
}


void EventLoop::AbortNotInLoopThread()
{
  cerr << "EventLoop::abortNotInLoopThread - EventLoop " << this
       << " was created in threadId_ = " << thread_id_
       << ", current thread id = " <<  this_thread::get_id() << endl;
}

TimerId EventLoop::RunAt(const define::SystemTimePoint time, const define::TimerCallback& cb)
{
    return timer_queue_->AddTimer(cb, time, 0);
}

TimerId EventLoop::RunAfter(long delay_ms, const define::TimerCallback& cb)
{
    auto expiration = std::chrono::system_clock::now() + std::chrono::milliseconds(delay_ms);
    return timer_queue_->AddTimer(cb, expiration, 0);
}

TimerId EventLoop::RunEvery(long interval_ms, const define::TimerCallback& cb)
{
    auto expiration = std::chrono::system_clock::now() + std::chrono::milliseconds(interval_ms);
    return timer_queue_->AddTimer(cb, expiration, interval_ms);
}