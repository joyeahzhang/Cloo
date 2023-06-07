#include "include/Socket.h"
#include "include/SocketAddress.h"

#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <exception>
#include <stdexcept>
#include <string>


using namespace Cloo;

Socket::Socket(SocketFd sock_fd) noexcept
    : sock_fd_(sock_fd)
{

}

Socket::~Socket() noexcept
{
    auto ret = ::close(static_cast<int>(sock_fd_));
    if(ret == -1)
    {
        // 不建议在析构函数中抛出异常, 这十分危险
        // std::string error_msg = "Failed to close socket: " + std::string(::strerror(errno));
        // throw std::runtime_error(error_msg);
        std::cerr <<  "Failed to close socket: " << ::strerror(errno);
    }
}

std::unique_ptr<Socket> Socket::CreateNonblockSocket()
{
    int sockfd = ::socket(AF_INET,
                    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                    IPPROTO_TCP);
    if(sockfd == -1 )
    {
        std::string error_msg = "Failed to create socket: " + std::string(::strerror(errno));
        throw std::runtime_error(error_msg);
    }
    return std::unique_ptr<Socket>(new Socket(static_cast<SocketFd>(sockfd)));
}

void Socket::Bind(const SocketAddress& addr)
{
    auto ret = ::bind(static_cast<int>(sock_fd_), 
                reinterpret_cast<const sockaddr*>(&addr.ToSockAddrIn()), sizeof(sockaddr_in));
    if(ret == -1)
    {
        std::string error_msg = "Failed to bind socket: " + std::string(::strerror(errno));
        throw std::runtime_error(error_msg);
    }
}

void Socket::Listen()
{
    auto ret = ::listen(static_cast<int>(sock_fd_), SOMAXCONN);
    if(ret == -1)
    {
        std::string error_msg = "Failed to listen socket: " + std::string(::strerror(errno));
        throw std::runtime_error(error_msg);
    }
}

SocketFd Socket::Accept(std::unique_ptr<SocketAddress>& peer_addr)
{
    sockaddr_in addr;
    socklen_t addr_len = sizeof(sockaddr_in);
    int ret = ::accept4(static_cast<int>(sock_fd_), reinterpret_cast<sockaddr*>(&addr), &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if( ret == -1)
    {
        auto saved_errno = errno;
        switch (saved_errno) 
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: 
            case EPERM:
            case EMFILE: 
                errno = saved_errno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                std::string error_msg = "Failed to accept4: " + std::string(::strerror(errno));
                throw std::runtime_error(error_msg);
        }
        peer_addr = nullptr;
    }
    else
    {
        peer_addr = std::make_unique<SocketAddress>(addr);
    }
    return static_cast<SocketFd>(ret);
}

void Socket::SetNonblockAndCloseOnExec()
{
    int flags = ::fcntl(static_cast<int>(sock_fd_), F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(static_cast<int>(sock_fd_), F_SETFL, flags);
    if(ret < 0)
    {
        std::string error_msg = "Failed to fcntl socket: " + std::string(::strerror(errno));
        throw std::runtime_error(error_msg);
    }

    flags = ::fcntl(static_cast<int>(sock_fd_), F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(static_cast<int>(sock_fd_), F_SETFD, flags);
    if(ret < 0)
    {
        std::string error_msg = "Failed to fcntl socket: " + std::string(::strerror(errno));
        throw std::runtime_error(error_msg);
    }
}

void Socket::SetReuseAddr(bool on)
{   
    int optval = on ? 1 : 0;
    auto ret = ::setsockopt(static_cast<int>(sock_fd_), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if(ret == -1)
    {
        std::string error_msg = "Failed to setsockopt: " + std::string(::strerror(errno));
        throw std::runtime_error(error_msg);
    }
}
