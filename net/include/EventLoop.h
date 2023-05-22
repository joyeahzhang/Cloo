#pragma once

#include <memory>
#include <thread>
#include <vector>

namespace Cloo 
{
// 前向声明简化头文件关系
class Poller;
class Channel;
class EventLoop final : public std::enable_shared_from_this<EventLoop>
{
public:
    EventLoop();
    ~EventLoop();
    // 不可拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop(const EventLoop&&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&&) = delete;
    
    void Init();

    // Loop是EventLoop的核心函数, 它几乎是一个不会停止的循环(除非主动调用Quit()使其退出)
    // Loop在每次迭代中都从IO多路复用组件中获取活动事件(由active_channels转发)
    // Loop可以在此线程中处理这些事件,也可以将它们交给其他线程处理
    void Loop();
    
    // 通过channel将 “某个fd” 和 “fd上的关心的IO事件” 注册到IO复用组件中
    // 之后IO多路复用组件中关于这个fd的活动事件都会通过channel转发给这个EventLoop
    void UpdateChannel(const std::shared_ptr<Channel>& channel);

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
    static std::shared_ptr<EventLoop> GetEventLoopOfThisThread();

    // 向EventLoop下达停止loop的命令, EventLoop会在下个loop迭代时停止循环
    // EventLoop可能不会立即停止loop, 因为它可能正在处理某个IO事件,需要耗时一段时间才能开始下一轮迭代
    void Quit()
    {
        quit_ = true;
    }

private:
    void abortNotInLoopThread();

    using ChannelList = std::vector<std::shared_ptr<Channel>>;
    bool looping_;
    bool quit_;
    std::thread::id thread_id_;
    // IO多路复用的组件
    // EventLoop拥有Poller的唯一所有权, 因此通过unique_ptr来管理
    std::unique_ptr<Poller> poller_;
    // 存放正在“转发”活跃IO事件的channel
    // 由poller_负责更新
    // Fixme: EventLoop应该拥有active_channels的唯一所有权, 但是这里似乎使用shared_ptr更加合适 
    std::shared_ptr<ChannelList> active_channels_;
};


} // namespance Cloo end