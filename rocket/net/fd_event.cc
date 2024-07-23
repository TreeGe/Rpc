#include <string.h>
#include <fcntl.h>  //设置非阻塞的头文件
#include "fd_event.h"
#include "rocket/common/log.h"

namespace rocket
{
    Fdevent::Fdevent(int fd) : m_fd(fd)
    {
        memset(&m_listen_event,0,sizeof(m_listen_event));
    }

    Fdevent::Fdevent()
    {
        memset(&m_listen_event,0,sizeof(m_listen_event));
    }

    Fdevent::~Fdevent()
    {
    }

    void Fdevent::setNoBlock()
    {
        int flag = fcntl(m_fd,F_GETFL,0);
        if(flag & O_NONBLOCK)
        {
            return;
        }

        fcntl(m_fd,F_SETFL,flag|O_NONBLOCK);
    }

    //上交任务?
    std::function<void()> Fdevent::handler(TriggerEvent event)    
    {
        if (event == TriggerEvent::IN_EVENT)
        {
            return m_read_callback;
        }
        else if(event==TriggerEvent::OUT_EVENT)
        {
            return m_write_callback;
        }
        else if(event==TriggerEvent::ERROR_EVENT)
        {
            return m_error_callback;
        }
        return nullptr;
    }

    //设置监听的事件
    void Fdevent::listen(TriggerEvent event_type, std::function<void()> callback , std::function<void()> error_callback)
    {
        if (event_type == TriggerEvent::IN_EVENT)
        {
            m_listen_event.events |= EPOLLIN;
            m_read_callback = callback;
        }
        else
        {
            m_listen_event.events |= EPOLLOUT;
            m_write_callback = callback;
        }

        if (m_error_callback == nullptr)
        {
            m_error_callback = error_callback;
        }
        else
        {
            m_error_callback=nullptr;
        }
        m_listen_event.data.ptr = this;  //把当前的fd事件指针送入事件中
    }

    void Fdevent::cancle(TriggerEvent event_type)
    {
        if (event_type == TriggerEvent::IN_EVENT)
        {
            m_listen_event.events &= (~EPOLLIN);
        }
        else
        {
            m_listen_event.events &= (~EPOLLOUT);
        }
    }
}