#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/common/log.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/fd_event_group.h"
#include "rocket/common/error_code.h"
#include "rocket/net/tcp/net_addr.h"


namespace rocket
{
    rocket::TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr)
    {
        m_event_loop = EventLoop::GetCurrentEventLoop();   //获得是一个新的进程的eventloop
        m_fd = socket(peer_addr->getFamily(),SOCK_STREAM,0);
        if(m_fd<0)
        {
            ERRORLOG("TcpClient::TcpClient() error , failed to create fd");
            return;
        }

        m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_fd);
        m_fd_event->setNoBlock();

        m_connection = std::make_shared<TcpConnection>(m_event_loop,m_fd,1024,m_peer_addr,nullptr,TcpConnectionByClinet);
        m_connection->setConnectionType(TcpConnectionByClinet);
    }
    TcpClient::~TcpClient()
    {
        DEBUGLOG("TcpClient::~TcpClient()");
        if(m_fd>0)
        {
            close(m_fd);
        }
    }
    void TcpClient::connect(std::function<void()> done)
    {
        int rt = ::connect(m_fd,m_peer_addr->getSockAddr(),m_peer_addr->getSockLen());
        if(rt==0)
        {
            initLocalAddr();
            DEBUGLOG("connect success");
            m_connection->setState(Connected);
            if(done)
            {
                done();
            }
        }
        else if(rt==-1)
        {
            if(errno==EINPROGRESS)
            {
                //epoll监听 可写事件  然后判断错误码
                m_fd_event->listen(Fdevent::OUT_EVENT,[this,done]()
                {
                    int rt = ::connect(m_fd,m_peer_addr->getSockAddr(),m_peer_addr->getSockLen());
                    if((rt<0&&errno==EISCONN)||rt==0)
                    {
                        DEBUGLOG("connect [%s] success",m_peer_addr->toString().c_str());
                        initLocalAddr();
                        m_connection->setState(Connected);
                    }
                    else
                    {
                        if(errno == ECONNREFUSED)
                        {
                            m_connect_err_code = ERROR_PEER_CLOSE;
                            m_connect_err_info = "connect refused, sys error =" + std::string(std::strerror(errno));
                        }
                        else
                        {
                            m_connect_err_code = ERROR_FAILED_CONNECT;
                            m_connect_err_info = "connect unknow error, sys error =" + std::string(std::strerror(errno));
                        }
                        ERRORLOG("connect error , errno = %d ,error = %s",errno,strerror(errno));
                        close(m_fd);
                        m_fd = socket(m_peer_addr->getFamily(),SOCK_STREAM,0);
                    }
                    // int error=0;
                    // socklen_t error_len = sizeof(error);
                    // getsockopt(m_fd,SOL_SOCKET,SO_ERROR,&error,&error_len);
                    // if(error==0)
                    // {
                    //     DEBUGLOG("connect [%s] success",m_peer_addr->toString().c_str());
                    //     initLocalAddr();
                    //     m_connection->setState(Connected);
                    // }
                    // else
                    // {
                    //     m_connect_err_code = ERROR_FAILED_CONNECT;
                    //     m_connect_err_info = "connect error, sys error ="+std::string(std::strerror(errno));
                    //     ERRORLOG("connect error , errno = %d ,error = %s",errno,strerror(errno));
                    // }
                    //连接完后需要去掉可写事件的监听  不然会一直触发
                    m_event_loop->deleteEpollEvent(m_fd_event);
                    DEBUGLOG("new begin to done");
                    //如果连接成功才会执行回调函数
                    if (done)
                    {
                        done();
                    }
                },[this,done]()
                {
                    if(errno == ECONNREFUSED)
                    {
                        m_connect_err_code = ERROR_FAILED_CONNECT;
                        m_connect_err_info = "connect refused, sys error =" + std::string(std::strerror(errno));
                        ERRORLOG("connect error , errno = %d ,error = %s", errno, strerror(errno));
                    }
                    else
                    {
                        m_connect_err_code = ERROR_FAILED_CONNECT;
                        m_connect_err_info = "connect unknow, sys error =" + std::string(std::strerror(errno));
                        ERRORLOG("connect error , errno = %d ,error = %s", errno, strerror(errno));
                    }
                });
                m_event_loop->addEpollEvent(m_fd_event);
                if(!m_event_loop->isLooping())
                {
                    m_event_loop->loop();
                }
            }
            else
            {
                //其他的错误码  直接打印错误
                m_connect_err_code = ERROR_FAILED_CONNECT;
                m_connect_err_info = "connect error, sys error =" + std::string(std::strerror(errno));
                ERRORLOG("connect error , errno = %d ,error = %s",errno,strerror(errno));
                if(done)
                {
                    done();
                }
            }
        }
 
    }
    void TcpClient::stop()//结束loop循环
    {
        if (m_event_loop->isLooping())
        {
            m_event_loop->stop();
        }
    }

    //异步的发送message
    //如果发送 message 成功 会调用done函数 函数的入参就是message对象
    void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done)
    {
        //1. 把message对象写入connection的buffer  done也写入
        //2. 启动connection可写事件

        m_connection->pushsendMessage(message,done);
        m_connection->listenWrite();

    }
    void TcpClient::readMessage(const std::string& req_id , std::function<void(AbstractProtocol::s_ptr)> done)
    {
        //1 . 监听可读事件
        //2 . 从buffer里 decode 得到message 对象  判断 是否 req_id 相等  相等则读成功 执行其回调
        m_connection->pushsendMessage(req_id,done);
        m_connection->listenRead();

    }

    int TcpClient::getConnectErrorCode()
    {
        return m_connect_err_code;
    }

    std::string TcpClient::getConnectErrorInfo()
    {
        return m_connect_err_info;
    }

    NetAddr::s_ptr TcpClient::getPeerAddr()
    {
        return m_peer_addr;
    }

    NetAddr::s_ptr TcpClient::getLocalAddr()
    {
        return m_local_addr;
    }

    void TcpClient::initLocalAddr()
    {
        sockaddr_in local_addr;
        socklen_t len=sizeof(local_addr);
        int ret = getsockname(m_fd,reinterpret_cast<sockaddr*>(&local_addr),&len);
        if(ret!=0)
        {
            ERRORLOG("initLocalAddr error, getsockname error, errno=%d,errno=%s",errno,strerror(errno));
            return;
        }

        m_local_addr = std::make_shared<IPNetAddr>(local_addr);
    }

    void TcpClient::addTimerEvent(TimeEvent::s_ptr timer_event)
    {
        m_event_loop->addTimerEvent(timer_event);
    }
}