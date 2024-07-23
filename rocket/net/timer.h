#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include <map>
#include <string.h>
#include "rocket/common/mutex.h"
#include "rocket/net/timer_event.h"
#include "rocket/net/fd_event.h"

namespace rocket
{
    class Timer : public Fdevent
    {
    public:
        Timer();

        ~Timer();

        void addtimerevent(TimeEvent::s_ptr event);

        void deltimerevent(TimeEvent::s_ptr event);

        void onTimer(); // 当发生了IO事件后 eventloop 会执行这个回调函数

    private:
        void resetArriveTimer();

    private:
        std::multimap<int64_t, TimeEvent::s_ptr> m_pending_events;
        Mutex m_mutex;
    };
}
#endif