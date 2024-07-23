#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H

#include <pthread.h>
#include <set>
#include <functional>
#include <queue>
#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "wakeup_fdevent.h"
#include "rocket/net/timer.h"

namespace rocket
{
    class EventLoop
    {
    public:
        EventLoop();
        ~EventLoop();

        void loop();  //一直循环

        void wakeup(); //唤醒 epoll_wait 去执行io操作

        void stop();  //停止循环

        void addEpollEvent(Fdevent *fdevent);   //添加

        void deleteEpollEvent(Fdevent *fdevent); //删除

        bool isInLoopThread();

        void addTask(std::function<void()> cb, bool is_wakeup = false);   //添加回调函数

        void addTimerEvent(TimeEvent::s_ptr event);

        bool isLooping();

        public:
        static EventLoop* GetCurrentEventLoop();

    private:
        void dealWakeup();

        void initWakeUpfdevent();

        void initTimer();

    private:
        pid_t m_thread_pid{0};  //记录线程号

        int m_epoll_fd{0};

        int m_wakeup_fd{0}; // 用于唤醒的套接字socket

        WakeupFdEvent *m_wakeupfd_event{NULL};

        bool m_stop_flag{false};     //停止循环的标志

        std::set<int> m_listen_fds;   //监听的所有的套接字

        std::queue<std::function<void()>> m_pending_tasks;

        Mutex m_mutex;

        Timer* m_timer {NULL};

        bool m_is_looping{false};
    };
}

#endif