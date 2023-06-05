#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace Cloo 
{

class EventLoop;

class EventLoopThread
{

public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;
    EventLoopThread(EventLoopThread&&) = delete;
    EventLoopThread& operator=(EventLoopThread&&) = delete;

    std::shared_ptr<EventLoop> StartLoop();

private:
    void ThreadFunc();

    std::shared_ptr<EventLoop> loop_;
    bool exited_;
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool thread_started_;
};

}