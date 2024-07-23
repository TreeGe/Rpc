#include <unistd.h>
#include <vector>
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/fd_event_group.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "tcp_connection.h"

namespace rocket
{
    rocket::TcpConnection::TcpConnection(EventLoop *event_loop, int fd, int buffer_size,NetAddr::s_ptr local_addr , NetAddr::s_ptr peer_addr , TcpConnectionType type /*= TcpConnectionByServer*/)
        : m_event_loop(event_loop),m_local_addr(local_addr), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd) ,m_connection_type(type) 
    {
        m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
        m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

        m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_fd);
        m_fd_event->setNoBlock();


        // m_coder = new StringCoder();
        m_coder = new TinypbCoder();

        if(m_connection_type == TcpConnectionByServer)
        {
            listenRead();
        }
    }

    rocket::TcpConnection::~TcpConnection()
    {
        INFOLOG("~TcpConnection");
        if(m_coder)
        {
            delete m_coder;
            m_coder=NULL;
        }
    }

    void rocket::TcpConnection::onRead()
    {
        // 1. 从socket缓冲区  调用系统的read函数 读取字节到 in_buffer里面
        if (m_state != Connected)
        {
            ERRORLOG("onRead error,client has already disconnected ,addr[%s] , clientf[%d] ,state[%s]", m_peer_addr->toString().c_str(), m_fd,std::to_string(m_state).c_str());
            return;
        }

        bool is_read_all = false;
        bool is_close = false;
        while (!is_read_all)
        {
            if (m_in_buffer->writeAble() == 0)  //扩容
            {
                m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
            }
            int read_count = m_in_buffer->writeAble();
            int write_index = m_in_buffer->getWriteIndex();

            int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
            // DEBUGLOG("success read bytes from addr[%s],client[%d]" ,m_peer_addr->toString().c_str(), m_fd);
            printf("success read bytes from addr,client");
            if (rt > 0)
            {
                printf("rt=%d\n",rt);
                m_in_buffer->moveWriteIndex(rt);
                if (rt == read_count)
                {
                    continue;
                }
                else if (rt < read_count)
                {
                    is_read_all = true;
                    break;
                }
                printf("rt=%d\n",rt);
            }
            else if(rt==0)
            {
                is_close = true;
                break;
            }
            else if(rt==-1 && errno == EAGAIN)
            {
                is_read_all=true;
                break;
            }
        }


        if (is_close)
        {
            // 处理关闭连接
            clear();
            INFOLOG("peer close, peer addr [%s], clientfd[%d] ", m_peer_addr->toString().c_str(), m_fd);
            return;
        }

        if (!is_read_all)
        {
            ERRORLOG("not read all data");
        }

        // TODO: 简答的echo 后面补充RCP 协议解析
        excute();
    }

    void rocket::TcpConnection::excute()
    {
        if (m_connection_type == TcpConnectionByServer)
        {
            std::vector<TinypbProtocol::s_ptr>result;
            std::vector<TinypbProtocol::s_ptr>reply_messages;
            m_coder->decoder(result,m_in_buffer);
            for(size_t i=0;i<result.size();i++)
            {
                //1.针对每一个请求 调用rpc方法  获取相应
                //2.将相应message放入发送缓冲区  监听可写事件回包
                INFOLOG("success get request[%s] from client [%s]", result[i]->m_msg_id.c_str(), m_peer_addr->toString().c_str());
               
                std::shared_ptr<TinypbProtocol> message = std::make_shared<TinypbProtocol>();
                // message->m_pb_data="hello. this is rocket rpc test data";
                // message->m_msg_id = result[i]->m_msg_id;
                RpcDispatcher::GetPrcDispatcher()->dispather(result[i], message,this);
                reply_messages.emplace_back(message);
            }
            printf("reply_messages.size()=%d",(int)reply_messages.size());
            m_coder->encoder(reply_messages,m_out_buffer);
            listenWrite();
        }
        else
        {
            //从buffer里 decode 得到message 对象  判断 是否 req_id 相等  相等则读成功 执行其回调
            std::vector<AbstractProtocol::s_ptr>result;
            m_coder->decoder(result,m_in_buffer);

            for(size_t i=0;i<result.size();i++)
            {
                std::string req_id = result[i]->m_msg_id;
                auto it = m_read_dones.find(req_id);
                if(it!=m_read_dones.end())
                {
                    it->second(result[i]);
                }
            }
        }
    }

    void rocket::TcpConnection::onWrite()
    {
        // 将当前out_buffer里面的输出全部发送给client
        if (m_state != Connected)
        {
            ERRORLOG("onWrite error,client has already disconnected ,addr[%s] , clientf[%d]", m_peer_addr->toString().c_str(), m_fd);
            return;
        }

        if(m_connection_type == TcpConnectionByClinet)
        {
            //1.将message 编码encoder 得到字节流
            //2.将数据写入buffer里面里面 然后全部发送
            std::vector<AbstractProtocol::s_ptr>messages;

            for(size_t i=0;i<m_write_dones.size();++i)
            {
                messages.push_back(m_write_dones[i].first);
            }

            m_coder->encoder(messages,m_out_buffer);
        }

        bool is_write_all = false;
        while (true)
        {
            if (m_out_buffer->readAble() == 0)
            {
                DEBUGLOG("no data need to send to client[%s]", m_peer_addr->toString().c_str());
                is_write_all = true;
                break;
            }
            int wirte_size = m_out_buffer->readAble();
            int read_index = m_out_buffer->gerReadIndex();
            int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), wirte_size);
            printf("wirte_size=%d\n",wirte_size);
            printf("read_index=%d\n",read_index);
            if (rt >= wirte_size) // 说明发送完了
            {
                printf("no data need to send to client");
                is_write_all = true;
                break;
            }
            if (rt == -1 && errno == EAGAIN)
            {
                // 发送缓冲区已满  不能再发送了
                // 这种情况我们等下次fd可写的时候再次发送数据即可
                DEBUGLOG("write data error,error = EGAIN and rt == -1");
                break;
            }
        }
        if(is_write_all)
        {
            m_fd_event->cancle(Fdevent::OUT_EVENT);
            m_event_loop->addEpollEvent(m_fd_event);  //这里是修改socket的状态
        }

        if(m_connection_type == TcpConnectionByClinet)
        {
            for(size_t i=0;i<m_write_dones.size();i++)
            {
                m_write_dones[i].second(m_write_dones[i].first);
            }
        }
        m_write_dones.clear();
    }

    void TcpConnection::setState(const TcpState state)
    {
        m_state=state;
    }

    TcpState TcpConnection::getState()
    {
        return m_state;
    }

    void TcpConnection::clear()
    {
        //处理一些关闭连接后的清理动作
        if(m_state == Closed)
        {
            return;
        }
        m_fd_event->cancle(Fdevent::IN_EVENT);
        m_fd_event->cancle(Fdevent::OUT_EVENT);

        m_event_loop->deleteEpollEvent(m_fd_event);
        // while(true){}

        m_state=Closed;
    }
    void TcpConnection::shutdown()
    {
        if(m_state==Closed||m_state==NotConnected)
        {
            return;
        }
        //处于半关闭
        m_state=HalfClosing;
        //调用shutdown 关闭读写 意味着服务器不会再对这个fd进行读写操作了
        //发送FIN报文  触发了四次挥手的第一个阶段
        //当fd发生可读事件  但是可读数据为0  即 对端发送了FIN
        ::shutdown(m_fd,SHUT_RDWR);
    }
    void TcpConnection::setConnectionType(TcpConnectionType type)
    {
        m_connection_type = type;
    }

    void rocket::TcpConnection::listenWrite()
    {
        m_fd_event->listen(Fdevent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
        m_event_loop->addEpollEvent(m_fd_event);
    }

    void TcpConnection::listenRead()
    {
        m_fd_event->listen(Fdevent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
        m_event_loop->addEpollEvent(m_fd_event);
    }

    void rocket::TcpConnection::pushsendMessage(AbstractProtocol::s_ptr message, std::function <void(AbstractProtocol::s_ptr)> done)
    {
        m_write_dones.push_back(std::make_pair(message,done));
    }
    void rocket::TcpConnection::pushsendMessage(const std::string &req_id, std::function<void(AbstractProtocol::s_ptr)> done)
    {
        m_read_dones.insert(std::make_pair(req_id,done));
    }

    NetAddr::s_ptr TcpConnection::getLocalAddr()
    {
        return m_local_addr;
    }

    NetAddr::s_ptr TcpConnection::getPeerAddr()
    {
        return m_peer_addr;
    }
}
