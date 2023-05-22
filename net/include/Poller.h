#pragma once

#include "EventLoop.h"
#include <vector>
#include <memory>
#include <chrono>
#include <map>

// forward-declaration
struct pollfd;

namespace Cloo 
{

class Channel;

class Poller
{

public:
    // Prefer "using-alias" to "typedef" in modern C++
    using ChannelList = std::vector<std::shared_ptr<Channel>>;
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

    Poller(const std::shared_ptr<EventLoop>& loop);
    ~Poller();

    // 不可拷贝
    Poller(const Poller&) = delete;
    Poller(Poller&&) = delete;
    Poller& operator=(const Poller&) = delete;
    Poller& operator=(Poller&&) = delete;
    
    // 调用poll(2)IO多路复用函数来管理到来的IO事件
    // 有需要处理的IO事件的fd所关联的Channel会被加入到activeChannels中
    // Notice: Poll必须在EventLoop所在的IO线程中被调用
    // Question: 如果不在EventLoop所在的IO线程中被调用会发生什么?
    TimePoint Poll(int timeoutMs, const std::shared_ptr<ChannelList>& activeChannels);
    
    // 通过Channel来更新和维护pollfds_列表, 过程中涉及多处修改, 详细内容见实现
    void UpdateChannel(const std::shared_ptr<Channel>& channel);

    void AssertInLoopTread()
    {
        ownerLoop_->AssertInLoopTread();
    }

private:  
    // 遍历pollfds_列表, 找出具有活动事件的fd, 把fd的活动事件填入关联的Channel, 然后把Channel填入到activeChannels中
    void fillActiveChannels(int numEvents, const std::shared_ptr<ChannelList>& activeChannels) const;

    using PollFdList = std::vector<pollfd>;
    using ChannelMap = std::map<int, std::shared_ptr<Channel>>; 
    // Poller所属的EventLoop
    std::shared_ptr<EventLoop> ownerLoop_;  
    // poll(2)所使用的文件描述符集合,所有需要有IO操作的文件描述符都会被注册到pollfds中
    PollFdList pollfds_; 
    // pollfds中fd所关联的channel的map, poll(2)返回的fd的IO事件会被注册到channel中,由channel“转发”给用户注册的回调函数
    ChannelMap channels_; 
};

}//end namespace Cloo