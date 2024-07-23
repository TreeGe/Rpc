#ifndef ROCKET_NET_WAKEUP_FDEVENT_H
#define ROCKET_NET_WAKEUP_FDEVENT_H
#include"fd_event.h"


namespace rocket
{
    class WakeupFdEvent : public Fdevent
    {
    public:
        WakeupFdEvent(int fd);
        ~WakeupFdEvent();

        void wakeup();


    private:

    };

}

#endif