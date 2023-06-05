#include "./include/EventLoopThread.h"
#include "./include/EventLoop.h"
#include <cassert>
#include <memory>
#include <mutex>


using namespace Cloo;
using namespace std;

EventLoopThread::EventLoopThread()
    : loop_(EventLoop::Create()),
      exited_(false),
      thread_started_(false)
{

}

EventLoopThread::~EventLoopThread()
{
    exited_ = true;
    loop_->Quit();
    if(thread_ && thread_->joinable())
    {
        thread_->join();
    }
}

shared_ptr<EventLoop> EventLoopThread::StartLoop()
{
    thread_ = make_unique<thread>(&EventLoopThread::ThreadFunc, this);
    unique_lock<mutex> lock(mutex_);
    cv_.wait(lock, [this]{return thread_started_;});
    
    return loop_;
}

void EventLoopThread::ThreadFunc()
{
    loop_ = EventLoop::Create();
    {
        unique_lock<mutex> lock(mutex_);
        thread_started_ = true;
        cv_.notify_one();
    }

    loop_->Loop();
}