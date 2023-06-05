#pragma once

#include "TimeDefs.h"
#include "CallbackDefs.h"

#include <chrono>
#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace Cloo 
{

// forward declaration
// 简化头文件包含关系
class TimerId;
class Timer;
class Channel;
class EventLoop;

// TimerQueue是一个定时器队列, 负责管理多个timer的定时任务.
// 用户只需要将设置好 callback , expiration , repeat_interval的timer
// 通过AddTimer方法加入到TimerQueue中, TimerQueue就会负责在timer到期时回调它注册的callback
// Notice: TimerQueue的定时精度并不高, 可以认为是ms级别

// TimerQueue的设计理念是将“定时事件”看作普通的IO事件, 统一交给IO多路复用组件来管理.
// 帮助我们实现这一点的key point是linux timerfd, 它是一个文件描述符，它允许您创建一个定时器,
// 我们总是将TimerQueue中接下来最早到期的Timer的过期时间作为timerfd的定时时间, 然后将timerfd加入到IO多路复用组件中。
// 当timerfd相关的定时器到期时，它将向文件描述符发送一个事件, IO多路复用组件可以感知到这个事件并通知用户。

// TimerQueue拥有四个成员变量, 分别是一个timerfd, 一个与timerfd绑定的Channel, 一个存放Timer的Set
// 和一个指向TimerQueue所属的EventLoop的指针。
// 在这四个成员变量中, TimerQueue不对任何一个变量拥有唯一所有权, 理由是: timerfd需要与IO多路复用组件共享, Channel需要
// 与EventLoop共享, Timer需要与用户共享, 执行EventLoop的指针更不必说。

class TimerQueue
{

public:
    
    TimerQueue(const std::shared_ptr<EventLoop>& loop);
    ~TimerQueue();

    // 不可拷贝与移动
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue(const TimerQueue&&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&&) = delete;

    // 添加一个定时器, 定时器会在when时间点回调用户设置的cb函数
    // 当interval_ms > 0 时表示这个定时器是循环触发的, 每次触发间隔为interval_ms
    // 即cb会在when,when+interval_ms,when+interval_ms*2...等时间点被调用
    // 这个函数可能被其他线程(非TimerQueue所属的IO线程)中被调用,因此必须做到线程安全
    TimerId AddTimer(const define::TimerCallback& cb, define::SystemTimePoint when, long interval_ms);
    void Cancel(const TimerId& timer_id);

private:

    void AddTimerInLoop(const std::shared_ptr<Timer>& timer);

    using Entry = std::pair<define::SystemTimePoint, std::shared_ptr<Timer>>;
    // 自定义的set的比较函数
    // C++17中inline修饰的static对象, 即使在header file中被定义, 也不会导致redefined问题
    using TimerList = std::set<Entry, std::function<bool(const Entry&,const Entry&)>>;
    inline static auto EntryComp = [](const std::pair<define::SystemTimePoint, std::shared_ptr<Timer>>& lhs, const std::pair<define::SystemTimePoint, std::shared_ptr<Timer>>& rhs)
    {
        if(lhs.first < rhs.first) return true;
        if(lhs.first > rhs.first) return false;
        return lhs.second.get() < rhs.second.get();
    };
    
    // 处理timerfd可读事件
    void HandleRead();
    std::vector<Entry> GetExpiration(define::SystemTimePoint now);
    // 将expired列表中已经到期的、循环触发的定时器重新放入timers_中
    void Reset(std::vector<Entry>& expired, define::SystemTimePoint now);
    bool Insert(const std::shared_ptr<Timer>& timer);

    std::weak_ptr<EventLoop> owner_loop_;
    const int timer_fd_;
    std::shared_ptr<Channel> timer_fd_channel_;
    TimerList timers_;

};

}