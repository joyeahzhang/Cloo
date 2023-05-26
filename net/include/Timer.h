#pragma once

#include "CallbackDefs.h"
#include "TimeDefs.h"
#include <chrono>
#include <functional>
namespace Cloo 
{

class Timer
{

public:
    // cb : 定时器到期时的回调函数
    // when : 定时器到期的时间点
    // interval_ms : 定时器循环间隔, 当interval_ms > 0 时, 定时器会循环触发, 触发节点为 when + n*interval_ms
    Timer(const define::TimerCallback& cb, define::SystemTimePoint when, long interval_ms);
    
    ~Timer();

    // 不可拷贝
    Timer(const Timer&) = delete;
    Timer(const Timer&&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer& operator=(const Timer&&) = delete;

    void Run() const { callback_(); };

    define::SystemTimePoint Expiration() const { return expiration_; }
    
    bool Repeat() const { return repeat_; }
    
    // 如果定时器是循环的(interval_ms > 0), 则重新开启一轮定时, timer.expiration_ = now + interval_ms
    // 如果定时器是不循环的(interval_ms < 0), 则不会开启新一轮定时, 并将timer.expiration_设置为无效值
    void Restart(define::SystemTimePoint now);

private:
    const define::TimerCallback callback_;
    define::SystemTimePoint expiration_;
    const long interval_ms_;
    const bool repeat_;
};

} // end namespace Cloo