#ifndef ROCKET_NET_TCP_ACCEPT_H
#define ROCKET_NET_TCP_ACCEPT_H

#include <memory.h>
#include "rocket/net/tcp/net_addr.h"

namespace rocket
{
    class TcpAcceptor
    {
        public:
        typedef std::shared_ptr<TcpAcceptor> s_ptr;

        TcpAcceptor(NetAddr::s_ptr local_addr);  //传入服务器的net地址

        ~TcpAcceptor();

        std::pair<int,NetAddr::s_ptr> accept();

        int getListenfd();

        private:
        NetAddr::s_ptr m_local_addr;           //服务器监听的地址  addr-> ip:port

        int m_listenfd{-1};  //监听套接字
        
        int m_family{-1};

    };
}

#endif