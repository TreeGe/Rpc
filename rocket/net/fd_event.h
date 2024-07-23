#ifndef ROCKET_NET_FDEVENT_H
#define ROCKET_NET_FDEVENT_H
#include <functional>
#include <sys/epoll.h>

namespace rocket
{
    class Fdevent
    {
    public:
        enum TriggerEvent    //事件的枚举类型
        {
            IN_EVENT = EPOLLIN,
            OUT_EVENT = EPOLLOUT,
            ERROR_EVENT = EPOLLERR,
        };

        Fdevent(int fd);

        Fdevent();

        ~Fdevent();

        void setNoBlock();

        std::function<void()> handler(TriggerEvent event_type);

        void listen(TriggerEvent event_type,std::function<void()>callback,std::function<void()> error_callback=nullptr);

        //取消监听
        void cancle(TriggerEvent event_type);

        int getFd()const{
            return m_fd;
        }

        epoll_event getEpollEvent()
        {
            return m_listen_event;
        }



    protected:
        int m_fd{-1};
        std::function<void()> m_read_callback{nullptr};
        std::function<void()> m_write_callback{nullptr};
        std::function<void()> m_error_callback{nullptr};

        epoll_event m_listen_event;  //记录监听的事件
    };
}

#endif