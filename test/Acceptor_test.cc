#include "../net/include/Acceptor.h"
#include "../net/include/EventLoop.h"
#include "../net/include/Socket.h"
#include "../net/include/SocketAddress.h"
#include <iostream>
#include <memory>
#include <unistd.h>

void NewConnectionCallback(Cloo::SocketFd fd, const Cloo::SocketAddress& peer)
{
    std::cout << "NewConnectionCallBack(): accepted a new connection from "
              << peer.ToHostPort() << std::endl;
    ::write(static_cast<int>(fd), "Hello World", 12);
    ::close(static_cast<int>(fd));
}

int main()
{
    Cloo::SocketAddress listen_addr {7777};
    auto loop = Cloo::EventLoop::Create();
    Cloo::Acceptor acceptor {loop, listen_addr};

    acceptor.SetNewConnectionCallback(NewConnectionCallback);
    acceptor.Listen();
    loop->Loop();
}