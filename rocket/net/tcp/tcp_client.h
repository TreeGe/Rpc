#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/timer_event.h"

namespace rocket
{
    class TcpClient
    {
        public:
        typedef std::shared_ptr<TcpClient> s_ptr;

        TcpClient(NetAddr::s_ptr peer_addr);

        ~TcpClient();

        //异步的进行 connct
        //如果connect完成 done会被执行
        void connect(std::function<void()> done);

        //异步的发送message
        //如果发送 message 成功 会调用done函数 函数的入参就是message对象
        void writeMessage(AbstractProtocol::s_ptr message,std::function<void(AbstractProtocol::s_ptr)> done);

        //异步的读取message
        //如果读取 message 成功 会调用done函数 函数的入参就是message对象  读编号为req_id的信息
        void readMessage(const std::string& req_id , std::function<void(AbstractProtocol::s_ptr)> done);

        void stop();//结束loop循环


        int getConnectErrorCode();

        std::string getConnectErrorInfo();

        NetAddr::s_ptr getPeerAddr();

        NetAddr::s_ptr getLocalAddr();

        void initLocalAddr();

        void addTimerEvent(TimeEvent::s_ptr timer_event);


        private:
        NetAddr::s_ptr m_peer_addr;  //对端的网络的地址

        NetAddr::s_ptr m_local_addr;

        EventLoop* m_event_loop{NULL};

        int m_fd{-1};

        Fdevent *m_fd_event{NULL};

        TcpConnection::s_ptr m_connection;

        int m_connect_err_code {0};
        std::string m_connect_err_info;

    };
}

#endif