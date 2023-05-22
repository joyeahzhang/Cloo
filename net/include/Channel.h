#pragma once
#include <functional>
#include <memory>

namespace Cloo 
{
// forward declaration 简化头文件关系
class EventLoop;

class Channel : public std::enable_shared_from_this<Channel>
{
// 在C++11后应该尽量使用using alias而非typedef
using EventCallBack = std::function<void()>;

public:

Channel(const std::shared_ptr<EventLoop>& loop, int fd);

void HandleEvent();
// 设置IO事件回调函数
void SetReadCallBack(const EventCallBack& cb) { readCallBack_ = cb; }
void SetWriteCallBack(const EventCallBack& cb) { writeCallBack_ = cb; }
void SetErrorCallBack(const EventCallBack& cb) { errorCallBack_ = cb; }
// 返回channel关联的文件描述符
int Fd() const { return fd_; }

int Events() const { return events_; }
void SetRevents(int revt) { revents_ = revt; }
bool IsNoneEvent() const { return events_ == kNoneEvent; }

void EnableReading() 
{ 
    events_ |= kReadEvent; 
    update(); 
}


int Index() const { return index_; }
void SetIndex(int index) { index_ = index; }

// channel所属的EventLoop
const std::shared_ptr<EventLoop>& OwnerLoop() const { return owner_loop_; };

private:
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    std::shared_ptr<EventLoop> owner_loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    EventCallBack readCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack errorCallBack_;
};

} // end namespace Cloo



