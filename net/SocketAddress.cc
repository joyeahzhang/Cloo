#include "include/SocketAddress.h"

#include <array>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <string>
#include <exception>
#include <system_error>
#include <arpa/inet.h>

using namespace Cloo;

SocketAddress::SocketAddress(uint16_t port)
{
    ::bzero(&sock_addr_, sizeof(sock_addr_));
    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr_.sin_port = htons(port);
}

SocketAddress::SocketAddress(std::string_view host, uint16_t port)
{
    ::bzero(&sock_addr_, sizeof sock_addr_);
    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_port = htons(port);
    if(::inet_pton(AF_INET, host.data(), &sock_addr_.sin_addr) <= 0)
    {
        std::string error_msg = "Fail to set sockaddr.sin_addr";
        throw std::runtime_error(error_msg);
    }
}

SocketAddress::SocketAddress(const sockaddr_in& addr)
        : sock_addr_(addr)
{

}


const sockaddr_in& SocketAddress::ToSockAddrIn() const noexcept
{
    return sock_addr_;
}

std::string SocketAddress::ToHostPort() const noexcept
{
    std::array<char,INET_ADDRSTRLEN> host { "INVALID" };
    ::inet_ntop(AF_INET, &sock_addr_.sin_addr, host.data(), host.size());
    uint16_t port = ntohs(sock_addr_.sin_port);
    return std::string(host.data()) + " : " + std::to_string(port);
}