#include "include/Channel.h"
#include "include/EventLoop.h"

#include <iostream>
#include <memory>
#include <poll.h>


using namespace Cloo;
using namespace std;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

// private constructor, 因为Channel类继承了enable_shared_ptr_from_this模板类
// 禁止用户通过构造函数创建栈上的对象
// 必须使用工厂方法Create(2)来创建堆上的对象,并初始化一个shared_ptr指针
Channel::Channel(const shared_ptr<EventLoop>& loop, int fd)
    : owner_loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1)
    {
        //do nothing
    }

shared_ptr<Channel> Channel::Create(const shared_ptr<EventLoop> &loop, int fd)
{
    shared_ptr<Channel> channel = shared_ptr<Channel>(new Channel(loop, fd));
    return channel;
}

void Channel::update()
{
    if(auto loop = owner_loop_.lock())
    {
        loop->UpdateChannel(shared_from_this());
    }
}

void Channel::HandleEvent()
{
    if(revents_ & POLLNVAL)
    {
        cerr << "Channel::HandleEvent() POLLNVAL" << endl;
    }

    if(revents_ & (POLLERR | POLLNVAL))
    {
        if(errorCallBack_) errorCallBack_();
    }

    if(revents_ & (POLLIN | POLLPRI | POLLHUP))
    {
        if(readCallBack_) readCallBack_();
    }

    if(revents_ & POLLOUT)
    {
        if(writeCallBack_) writeCallBack_();
    }
}