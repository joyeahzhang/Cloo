#include "include/Channel.h"
#include "include/EventLoop.h"
#include <poll.h>

#include <iostream>

using namespace Cloo;
using namespace std;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(const shared_ptr<EventLoop>& loop, int fd)
    : owner_loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1)
    {
        //do nothing
    }

void Channel::update()
{
    owner_loop_->UpdateChannel(shared_from_this());
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