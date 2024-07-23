#ifndef ROCKET_TCP_TCP_CONNECTION_H
#define ROCKET_TCP_TCP_CONNECTION_H

#include <queue>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/io_thread.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
namespace rocket
{

    enum TcpState
    {
        NotConnected = 1,
        Connected = 2,
        HalfClosing = 3,
        Closed = 4,
    };

    enum TcpConnectionType
    {
        TcpConnectionByServer = 1 ,//作为服务端使用  代表跟对端客户端的连接
        TcpConnectionByClinet = 2 ,//作为客户端使用  代表跟对端服务器连接
    };

    class RpcDispatcher;

    class TcpConnection
    {
    public:
        typedef std::shared_ptr<TcpConnection> s_ptr;

        TcpConnection(EventLoop *eventloop, int fd, int buffer_size,NetAddr::s_ptr local_addr, NetAddr::s_ptr peer_addr , TcpConnectionType type = TcpConnectionByServer);

        ~TcpConnection();

        void onRead();

        void excute();

        void onWrite();

        void setState(const TcpState state);

        TcpState getState();

        void clear();//清除连接

        void shutdown();//服务器主动关闭连接

        void setConnectionType(TcpConnectionType type);

        //启动监听可写事件
        void listenWrite();

        //启动监听可读事件、
        void listenRead();

        void pushsendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

        void pushsendMessage(const std::string& req_id, std::function<void(AbstractProtocol::s_ptr)> done);

        NetAddr::s_ptr getLocalAddr();

        NetAddr::s_ptr getPeerAddr();
    private:
        EventLoop *m_event_loop{NULL}; // 代表持有该连接的IO线程

        NetAddr::s_ptr m_local_addr; // 本地Ip地址
        NetAddr::s_ptr m_peer_addr;  // 客户端IP地址

        TcpBuffer::s_ptr m_in_buffer;  // 接收缓冲区
        TcpBuffer::s_ptr m_out_buffer; // 发送缓冲区

        Fdevent *m_fd_event{NULL};

        AbstractCoder *m_coder{NULL};
        TcpState m_state;
        int m_fd{0};

        TcpConnectionType m_connection_type {TcpConnectionByServer};

        //std :: pair<AbstractProtocol::s_ptr , std::function<void(AbstractProtocol::s_ptr >
        std::vector<std::pair<AbstractProtocol::s_ptr , std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones;

        //key为req_id
        std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_dones;

    };
}

#endif