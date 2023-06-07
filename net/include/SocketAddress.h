#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <string_view>

namespace Cloo 
{

class SocketAddress
{
public:
    SocketAddress(uint16_t port);
    SocketAddress(std::string_view host, uint16_t port);
    SocketAddress(const sockaddr_in& addr);

    SocketAddress(const SocketAddress&) = delete;
    SocketAddress& operator=(const SocketAddress&) = delete;

    const sockaddr_in& ToSockAddrIn() const noexcept;

    std::string ToHostPort() const noexcept;
    
private:
    sockaddr_in sock_addr_;
};

} // end namespace Cloo