#pragma once

#include <memory>

namespace Cloo 
{

class SocketAddress;

// 使用枚举保证类型安全
enum class SocketFd
{
    invalid = -1
};

class Socket
{
public:
    static std::unique_ptr<Socket> CreateNonblockSocket();
    
    void Bind(const SocketAddress& sock_addr);
    void Listen();
    SocketFd Accept(std::unique_ptr<SocketAddress>& peer_addr);
    void SetNonblockAndCloseOnExec();
    void SetReuseAddr(bool on);

    SocketFd Fd() const {return sock_fd_;}
    ~Socket() noexcept;
private:
    explicit Socket(SocketFd sock_fd) noexcept;
    SocketFd sock_fd_;
};

}