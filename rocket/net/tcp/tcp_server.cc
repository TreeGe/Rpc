#include "rocket/net/tcp/tcp_server.h"
#include "rocket/common/log.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"
#include <memory>

namespace rocket
{
    TcpServer::TcpServer(NetAddr::s_ptr m_local) : m_local_addr(m_local)
    {
        Init();
        INFOLOG("rocket TcpServer listen succes on [%s]", m_local_addr->toString().c_str());
    }
    TcpServer::~TcpServer()
    {
        if (m_main_event_loop)
        {
            delete m_main_event_loop;
            m_main_event_loop = NULL;
        }
    }

    void TcpServer::start()
    {
        m_io_thread_group->start(); // 开启IO线程
        m_main_event_loop->loop();  // 主线程一直阻塞在这里
    }

    void rocket::TcpServer::Init()
    {
        m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr);
        m_main_event_loop = EventLoop::GetCurrentEventLoop();
        m_io_thread_group = new IOThreadGroup(2); // 2个Io线程
        m_listen_fd_event = new Fdevent(m_acceptor->getListenfd());
        m_listen_fd_event->listen(Fdevent::IN_EVENT,std::bind(&TcpServer::onAccept,this));     
        m_main_event_loop->addEpollEvent(m_listen_fd_event);                 //将监听实现绑定到fdevent上
    }

    void TcpServer::onAccept()
    {
        auto re=m_acceptor->accept();
        int client_fd = re.first;
        NetAddr::s_ptr peer_addr = re.second;

        m_client_counts++;

        //把client_fd添加到任意IO线程里
        // m_io_thread_group->getIOThread()->getEventLoop()->addEpollEvent();

        IOThread* io_thread = m_io_thread_group->getIOThread();     //获取io对象
        TcpConnection::s_ptr connect = std::make_shared<TcpConnection>(io_thread->getEventLoop(),client_fd,128,peer_addr,m_local_addr);
        connect->setState(Connected);
        m_client.insert(connect);

        INFOLOG("TcpServer success get client ,fd=%d",client_fd);
    }
}
