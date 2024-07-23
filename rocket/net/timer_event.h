#ifndef ROCKET_NET_TIMER_EVENT_H
#define ROCKET_NET_TIMER_EVENT_H

#include <functional>
#include <memory>

namespace rocket
{
    class TimeEvent
    {
    public:
        typedef std::shared_ptr<TimeEvent> s_ptr;

        TimeEvent(int interval, bool is_repeated, std::function<void()> cb);
        ~TimeEvent();

        int64_t getArriveTime()
        {
            return m_arrive_time;
        }

        void setCancle(bool value)
        {
            m_is_cancle= value;
        }

        bool isCancled()
        {
            return m_is_cancle;
        }

        bool isReapted()
        {
            return m_is_repeated;
        }

        std::function<void()> getCallBack()
        {
            return m_task;
        }

        void resetArriveTime();
    private:
        int64_t m_arrive_time; // ms
        int64_t m_interval;    // ms
        bool m_is_repeated{false};
        bool m_is_cancle{false};

        std::function<void()> m_task;
    };

}
#endif