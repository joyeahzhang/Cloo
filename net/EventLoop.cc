#include "include/EventLoop.h"
#include <cstdlib>
#include <iostream>
#include <cassert>

using namespace std;
using namespace Cloo;

thread_local EventLoop* T_LOOP_IN_THIS_THREAD = nullptr;

EventLoop::EventLoop() : looping_(false), thread_id_(this_thread::get_id())
{
    cout<<"EventLoop created " << this << " in thread " << thread_id_ <<endl;
    if(T_LOOP_IN_THIS_THREAD)
    {
        cerr<<"Another EventLoop " << T_LOOP_IN_THIS_THREAD << " exists in this thread " << thread_id_ << endl;
        abort();
    }
    else
    {
        T_LOOP_IN_THIS_THREAD = this;
    }
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

    // do something

    looping_ = false;
}

EventLoop* EventLoop::GetEventLoopOfThisThread()
{
    return T_LOOP_IN_THIS_THREAD;
}


