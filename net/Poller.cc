#include "include/Poller.h"
#include "include/Channel.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <poll.h>
#include <sys/poll.h>

#include <iostream>

using namespace Cloo;
using namespace std;

Poller::Poller(const shared_ptr<EventLoop>& loop)
    : ownerLoop_(loop)
{

}

Poller::~Poller()
{

}

Poller::TimePoint Poller::Poll(int timeoutMs, const std::shared_ptr<ChannelList>& activeChannels)
{
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    auto now = chrono::system_clock::now();
    if(numEvents > 0)
    {
        cout << numEvents << " events happened" << endl; 
        fillActiveChannels(numEvents, activeChannels);
    }
    else if(numEvents == 0)
    {
        cout << "nothing happened" <<endl;
    }
    else
    {
        cerr << "Poller::poll() error" <<endl;
    }
    return now;
}


void Poller::fillActiveChannels(int numEvents, const shared_ptr<ChannelList>& activeChannels) const
{
    for(auto iter = pollfds_.begin(); iter != pollfds_.end() && numEvents > 0; iter++)
    {
        if(iter->revents > 0)
        {
            // 在numEvents == 0时结束循环, 避免在已经处理完所有poll返回的pollfd后
            // 对pollfds做无效地遍历
            --numEvents;
            assert(channels_.count(iter->fd) != 0);
            auto channel = channels_.find(iter->fd)->second;
            assert(channel->Fd() == iter->fd);
            channel->SetRevents(iter->revents);
            activeChannels->push_back(channel);
        }
    }
}

// 三处update:
//  update Poller::pollfds : 将channel携带的fd和“关心的fd的IO事件”更新到Poller::pollfds中
//  update Poller::channels : 将<channel->fd, channel>的KV更新到Poller::channels
//  update channel: 将channel->fd插入到pollfds数组的最新index更新到channel->index 
// 函数会严格要求Poller::channels[fd] - channel - Poller::pollfds[index]三者对应关系的正确性
// 基本的关系为：
//  Poller::channels[channel->fd] = channel
//  Poller::pollfds[channel->index].fd = channel->fd 
void Poller::UpdateChannel(const shared_ptr<Channel>& channel)
{
    AssertInLoopTread();
    // channel->Index < 0代表channel从未被注册到Poller中
    // 需要将channel和对应的fd分别注册到Poller::channels_和Poller::pollfds_中
    if(channel->Index() < 0)
    {
        assert(channels_.count(channel->Fd()) == 0);
        pollfd pfd;
        pfd.fd = channel->Fd();
        pfd.events = static_cast<short>(channel->Events());
        pfd.revents = 0;
        pollfds_.push_back(pfd); // vector push_back()均摊分析的时间复杂度为O(1)
        int idx = static_cast<int>(pollfds_.size()) - 1 ;
        channel->SetIndex(idx);
        channels_[pfd.fd] = channel;
    }
    // channel曾经已经注册到Poller中,这次只需要更新
    else
    {
        assert(channels_.count(channel->Fd()) != 0);
        assert(channels_[channel->Fd()] == channel);
        int idx = channel->Index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        pollfd& pfd = pollfds_[channel->Index()];
        assert(pfd.fd == channel->Fd() || pfd.fd == -1);
        pfd.events = static_cast<short>(channel->Events());
        pfd.revents = 0;
        // Channel不关注任何IO事件, 将对应的fd置为-1, poll(2)会忽略此项
        if (channel->IsNoneEvent())
        {
            pfd.fd = -1;
        }
    }
}