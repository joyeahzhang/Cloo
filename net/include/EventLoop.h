#pragma once
#include <thread>

namespace Cloo 
{

class EventLoop final
{
public:
    EventLoop();
    ~EventLoop();
    // 不可拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop(const EventLoop&&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&&) = delete;
    void Loop();
    
    // assert(EventLoop处在thread_id_指向的线程中)
    inline void AssertInLoopTread()
    {
        if(!IsInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

    // 判断EventLoop是否处在thread_id_指向的线程中
    bool IsInLoopThread() const
    {
        return thread_id_ == std::this_thread::get_id();
    }

    // 得到当前线程中的EventLoop, 如果当前线程没有创建EventLoop则会返回nullptr
    static EventLoop* GetEventLoopOfThisThread();

private:
    void abortNotInLoopThread();

    bool looping_;
    std::thread::id thread_id_;
};


} // namespance Cloo end