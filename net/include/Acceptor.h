#pragma once


#include "Socket.h"
#include "SocketAddress.h"
#include <functional>
#include <memory>
namespace Cloo 
{

class EventLoop;
class Channel;
enum class SocketFd;

class Acceptor
{
using NewConnectionCallback = std::function<void (SocketFd, const SocketAddress&)> ;

public:
    Acceptor(std::shared_ptr<EventLoop> owner_loop, const SocketAddress& listen_addr);

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    void SetNewConnectionCallback(const NewConnectionCallback& cb) {cb_ = cb; };
    void Listen();
    void HandleRead();
    bool Listenning() const { return listenning_; }

private:
    std::weak_ptr<EventLoop> owner_loop_;
    std::unique_ptr<Socket> accept_socket_;
    std::shared_ptr<Channel> channel_;
    NewConnectionCallback cb_;
    bool listenning_;
};

} // end namespace Cloo