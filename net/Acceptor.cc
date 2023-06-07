#include "include/Acceptor.h"
#include "include/EventLoop.h"
#include "include/Channel.h"
#include "include/Socket.h"
#include "include/SocketAddress.h"
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <unistd.h>


using namespace Cloo;

Acceptor::Acceptor(std::shared_ptr<EventLoop> owner_loop, const SocketAddress& listen_addr)
    : owner_loop_(owner_loop),
      accept_socket_(Socket::CreateNonblockSocket()),
      channel_(Channel::Create(owner_loop, static_cast<int>(accept_socket_->Fd()))),
      listenning_(false)
{
    accept_socket_->SetReuseAddr(true);
    accept_socket_->Bind(listen_addr);
    channel_->SetReadCallBack(std::bind(&Acceptor::HandleRead,this));
}


void Acceptor::Listen()
{
    if(auto loop = owner_loop_.lock())
    {
        loop->AssertInLoopTread();
    }
    listenning_ = true;
    accept_socket_->Listen();
    channel_->EnableReading();
} 

void Acceptor::HandleRead()
{
    if(auto loop = owner_loop_.lock())
    {
        loop->AssertInLoopTread();
    }

    std::unique_ptr<SocketAddress> peer_addr;
    SocketFd conn_fd; 
    try 
    {
        conn_fd = accept_socket_->Accept(peer_addr);
    } catch (std::exception&) 
    {
        abort();
    }
    
    if(conn_fd != SocketFd::invalid)
    {
        if(cb_)
        {
            cb_(conn_fd, *peer_addr);
        }
        else
        {
            ::close(static_cast<int>(conn_fd));
        }
    }
}