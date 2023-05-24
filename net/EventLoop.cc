#include "include/EventLoop.h"
#include "include/Poller.h"
#include "include/Channel.h"
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <memory>
#include <algorithm>
#include <thread>

using namespace std;
using namespace Cloo;

// 当前线程所持有的EventLoop对象指针
thread_local shared_ptr<EventLoop> T_LOOP_IN_THIS_THREAD = nullptr;
// Poll超时时间(ms)
const int K_POLL_TIMEOUT_MS = 10000;

std::shared_ptr<EventLoop> EventLoop::Create()
{
    auto loop = make_shared<EventLoop>();
    loop->looping_ = false;
    loop->quit_ = false;
    loop->thread_id_ = this_thread::get_id();
    loop->poller_ = make_unique<Poller>(loop);
    loop->active_channels_ = make_shared<ChannelList>();

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

void EventLoop::Init()
{
    looping_ = false;
    quit_ = false;
    thread_id_ = this_thread::get_id();
    poller_ = make_unique<Poller>(shared_from_this());
    active_channels_ = make_shared<ChannelList>();
    

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
        std::for_each(active_channels_->begin(), active_channels_->end(), [&](const auto& channel){channel->HandleEvent();});
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


void EventLoop::abortNotInLoopThread()
{
  cerr << "EventLoop::abortNotInLoopThread - EventLoop " << this
       << " was created in threadId_ = " << thread_id_
       << ", current thread id = " <<  this_thread::get_id() << endl;
}
