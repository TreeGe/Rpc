#include <sys/timerfd.h>
#include "rocket/net/timer.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket
{
    rocket::Timer::Timer() : Fdevent()
    {
        m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        DEBUGLOG("timer fd = %d", m_fd);
        // printf("timer fd = %d\n", m_fd);

        // 把fd上的可读放到了eventloop上监听
        listen(Fdevent::IN_EVENT, std::bind(&Timer::onTimer, this));
    }

    rocket::Timer::~Timer()
    {
    }

    void Timer::resetArriveTimer()
    {
        ScopeMutex<Mutex> lock(m_mutex);
        auto tmp = m_pending_events;
        lock.unlock();

        if (tmp.empty())
        {
            return;
        }
        int64_t now = getNowMs();

        auto it = tmp.begin();
        int64_t interval = 0;
        if (it->second->getArriveTime() > now)
        {
            interval = it->second->getArriveTime() - now;
        }
        else
        {
            interval = 100;
        }
        timespec ts;
        memset(&ts,0,sizeof(ts));
        ts.tv_sec = interval / 1000;
        ts.tv_nsec = (interval % 1000) * 1000000;

        itimerspec value;
        memset(&value,0,sizeof(value));
        value.it_value = ts;

        int rt = timerfd_settime(m_fd, 0, &value, NULL);
        if (rt != 0)
        {
            ERRORLOG("timerfd_settime error,errno=%d,error=%s", errno, strerror(errno));
        }
        // DEBUGLOG("timer reset to %lld",now+interval);
        // printf("timer reset to %ld\n",now+interval);
    }

    void rocket::Timer::addtimerevent(TimeEvent::s_ptr event)
    {
        bool is_reset_timerfd = false;

        ScopeMutex<Mutex> lock(m_mutex);
        if (m_pending_events.empty())
        {
            is_reset_timerfd = true;
        }
        else
        {
            auto it = m_pending_events.begin(); // 取出执行事件最早的一个事件
            if ((*it).second->getArriveTime() > event->getArriveTime())
            {
                is_reset_timerfd = true;
            }
        }
        m_pending_events.emplace(event->getArriveTime(), event);
        lock.unlock();

        if (is_reset_timerfd)
        {
            resetArriveTimer();
        }
    }

    void rocket::Timer::deltimerevent(TimeEvent::s_ptr event)
    {
        event->setCancle(true);

        ScopeMutex<Mutex> lock(m_mutex);

        auto begin = m_pending_events.lower_bound(event->getArriveTime()); // 第一个key的值等于event->getArrveTImer()
        auto end = m_pending_events.upper_bound(event->getArriveTime());   // 最后一个

        auto it = begin;
        for (it = begin; it != end; it++)
        {
            if (it->second == event)
            {
                break;
            }
        }

        if (it != end)
        {
            m_pending_events.erase(it);
        }

        lock.unlock();
        // DEBUGLOG("success delete TimerEvent at arrive time %lld", event->getArriveTime());
        printf("success delete TimerEvent at arrive time %ld\n", event->getArriveTime());
    }

    // 作用是到了预定的事件执行回调函数
    void rocket::Timer::onTimer()
    {
         // 处理缓冲区数据  防止下一次继续出发可读事件
        char buf[8];
        while (1)
        {
            if ((read(m_fd, buf, 8) == -1) &&errno == EAGAIN)
            {
                break;
            }
        }

        // 执行定时事件
        int64_t now = getNowMs();
        DEBUGLOG("now time =%lld",now);

        std::vector<TimeEvent::s_ptr> tmps;
        std::vector<std::pair<int64_t, std::function<void()>>> tasks;

        ScopeMutex<Mutex> lock(m_mutex);
        auto it = m_pending_events.begin();

        for (it = m_pending_events.begin(); it != m_pending_events.end(); it++)
        {
            if ((*it).first <= now)
            {
                if (!(*it).second->isCancled())
                {
                    tmps.push_back((*it).second);
                    tasks.push_back(std::make_pair((*it).second->getArriveTime(), (*it).second->getCallBack()));
                }
            }
            else
            {
                break;
            }
        }

        m_pending_events.erase(m_pending_events.begin(),it);
        lock.unlock();

        //需要把重复的Event 再次添加进去
        for(auto i = tmps.begin();i!=tmps.end();i++)
        {
            if((*i)->isReapted())
            {
                //调整arriveTime
                (*i)->resetArriveTime();
                addtimerevent(*i);
            }
        }

        resetArriveTimer();

        for(auto i:tasks)
        {
            if(i.second)
            {
                i.second();
            }
        }
    }

}