#include "eventloop.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <string.h>
#include <sys/epoll.h>

#define ADD_TO_EPOLL()                                                                       \
    auto it = m_listen_fds.find(event->getFd());                                             \
    int op = EPOLL_CTL_ADD;                                                                  \
    if (it != m_listen_fds.end())                                                            \
    {                                                                                        \
        op = EPOLL_CTL_MOD;                                                                  \
    }                                                                                        \
    epoll_event tmp = event->getEpollEvent();                                                \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);                                \
    if (rt == -1)                                                                             \
    {                                                                                        \
        ERRORLOG("failed epoll_ctl when add fd,errno=%s, error=%s", errno, strerror(errno)); \
    }                                                                       \
    m_listen_fds.insert(event->getFd());                         \
    DEBUGLOG("add event success, fd[%d]",event->getFd());    \

#define DELETE_TO_EPOLL()   \
            auto it = m_listen_fds.find(event->getFd());  \
            if (it == m_listen_fds.end())  \
            {  \
                return;   \
            }   \
            int op = EPOLL_CTL_DEL;   \
            epoll_event tmp = event->getEpollEvent();   \
            int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), NULL);   \
            if (rt == -1)    \
            {     \
                ERRORLOG("failed epoll_ctl when delete fd,errno=%s, error=%s",errno,strerror(errno));   \
            }    \
            m_listen_fds.erase(event->getFd());                                          \
            DEBUGLOG("delete event success, fd[%d]",event->getFd());    \

namespace rocket
{

    static thread_local EventLoop *t_current_eventloop = NULL;  //当前线程的eventloop
    static int g_epoll_max_timeout = 10000;   //10秒
    static int g_epoll_max_event = 10;

    EventLoop::EventLoop()
    {
        // 之前创建过  异常  直接退出
        if (t_current_eventloop != NULL)
        {
            ERRORLOG("failed to create event loop, this thread has create event loop");
            exit(0);
        }

        m_thread_pid = getThreadId();
        m_epoll_fd = epoll_create(10);

        if (m_epoll_fd == -1)
        {
            ERRORLOG("failed to create event loop, epoll_create,error info[%d]", errno);
            exit(0);
        }

        initWakeUpfdevent();
        initTimer();
        // INFOLOG();
        INFOLOG("succese create event loop in thread %d \n", m_thread_pid);
        // printf("succese create event loop in thread %d \n", m_thread_pid);
        t_current_eventloop = this;
    }

    EventLoop::~EventLoop()
    {
        close(m_epoll_fd);
        if(m_wakeupfd_event)
        {
            delete m_wakeupfd_event;
            m_wakeupfd_event=NULL;
        }
        if(m_timer)
        {
            delete m_timer;
            m_timer=NULL;
        }
    }

    void EventLoop::loop()
    {
        m_is_looping = true;
        while (!m_stop_flag)
        {
            // 先执行任务 再使用epoll等待 IO事件
            ScopeMutex<Mutex> lock(m_mutex);
            std::queue<std::function<void()>> tmp_tasks ;
            m_pending_tasks.swap(tmp_tasks);
            lock.unlock();
            while (!tmp_tasks.empty())
            {
                std::function<void()> cb = tmp_tasks.front(); // 调用函数
                tmp_tasks.pop();      //弹出函数
                if(cb)
                {
                    cb();
                }
            }

            //如果有定时任务需要执行,那么执行
            //1.怎么判断一个定时任务需要执行(now() > TImeEvent.arrtive time)
            //2. arrtive_time 如何让eventloop监听

            int timeout = g_epoll_max_timeout;      //最大超市时间
            epoll_event result_event[g_epoll_max_event];   //用于存放epoll_wait得到的事件   最大的见监听事件
            int rt = epoll_wait(m_epoll_fd, result_event, g_epoll_max_event, timeout);  //rt是有事件发射的socket的个数
            printf("now end epoll_wait, rt=%d", rt);

            if (rt < 0)
            {
                ERRORLOG("epoll_wait error, errno=", errno);
            }
            else
            {
                //处理IO事件
                for (int i = 0; i < rt; ++i)
                {
                    epoll_event trigger_event = result_event[i];
                    Fdevent* fd_event = static_cast<Fdevent*>(trigger_event.data.ptr);
                    if(fd_event==NULL)
                    {
                        continue;
                    }

                    if (trigger_event.events & EPOLLIN)
                    {
                        addTask(fd_event->handler(Fdevent::IN_EVENT));
                    }
                    if (trigger_event.events & EPOLLOUT)
                    {
                        addTask(fd_event->handler(Fdevent::OUT_EVENT));
                    }
                    // if(!(trigger_event.events & EPOLLIN)&&!trigger_event.events & EPOLLOUT)
                    // {
                    //     int event = trigger_event.events;
                    //     DEBUGLOG("unknow event = %d", event);
                    // }

                    //EPOLLHUP EPOLLERR

                    if(trigger_event.events & Fdevent::ERROR_EVENT)
                    {
                        //删除出错的套接字
                        deleteEpollEvent(fd_event);
                        DEBUGLOG("fd %d trigger EPOLLERROR event ",fd_event->getFd());
                        if(fd_event->handler(Fdevent::ERROR_EVENT)!=nullptr)
                        {
                            addTask(fd_event->handler(Fdevent::OUT_EVENT));
                        }
                    }
                }
            }
        }
    }

    void EventLoop::wakeup()
    {
        m_wakeupfd_event->wakeup();
    }

    void EventLoop::stop()
    {
        m_stop_flag = true;
        wakeup();
    }

    void EventLoop::dealWakeup()
    {
    }

    void EventLoop::initWakeUpfdevent()
    {
        m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
        INFOLOG("wakeup fd = %d",m_wakeup_fd);
        if (m_wakeup_fd < 0)
        {
            ERRORLOG("failed to create event loop,epoll_create,error info[%d]", errno);
            exit(0);
        }
        m_wakeupfd_event =new WakeupFdEvent(m_wakeup_fd);

        m_wakeupfd_event->listen(Fdevent::IN_EVENT,[this]()
        {
            char buf[8];
            while(read(m_wakeup_fd,buf,8)!=-1 && errno !=EAGAIN)
            {

            }
            DEBUGLOG("read full bytes from wakeup fd");
            // printf("read full bytes from wakeup fd[%d]\n",m_thread_pid);
        });

        addEpollEvent(m_wakeupfd_event);
    }

    void EventLoop::initTimer()
    {
        // printf("初始化了timer\n");
        m_timer = new Timer();
        addEpollEvent(m_timer);
    }

    void EventLoop::addTask(std::function<void()> cb, bool is_wakeup)
    {
        ScopeMutex<Mutex>lock(m_mutex);
        m_pending_tasks.push(cb);
        lock.unlock();
        if(is_wakeup)
        {
            wakeup();
        }
    }

    void EventLoop::addTimerEvent(TimeEvent::s_ptr event)
    {
        m_timer->addtimerevent(event);
    }

    bool EventLoop::isLooping()
    {
        return m_is_looping;
    }

    EventLoop *EventLoop::GetCurrentEventLoop()
    {
        if(t_current_eventloop)
        {
            return t_current_eventloop;
        }
        t_current_eventloop = new EventLoop();
        return t_current_eventloop;
    }

    void EventLoop::addEpollEvent(Fdevent *event)
    {
        //进程在当前线程内 直接添加到epoll中
        if (isInLoopThread())
        {
            ADD_TO_EPOLL();
        }
        else  //不在当前进线程中  放入回调函数中
        {
            auto cb = [this, event]()   
            {
                ADD_TO_EPOLL();
            };//返回值是一个函数
            addTask(cb,true);
        }
    }

    void EventLoop::deleteEpollEvent(Fdevent *event)
    {
        //在当前线程里
        if(isInLoopThread())
        {
            DELETE_TO_EPOLL();
        }
        else   //不在当前线程里
        {
            auto cb=[this,event]()
            {
                DELETE_TO_EPOLL();
            };
            addTask(cb,true);   //添加到任务队列
        }
    }

    bool EventLoop::isInLoopThread()
    {
        return getThreadId()==m_thread_pid;
    }


}