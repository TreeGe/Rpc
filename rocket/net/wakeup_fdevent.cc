#include "wakeup_fdevent.h"
#include "rocket/common/log.h"
#include <unistd.h>
namespace rocket
{
    WakeupFdEvent::WakeupFdEvent(int fd) : Fdevent(fd)
    {
    }

    WakeupFdEvent::~WakeupFdEvent()
    {
    }


    void WakeupFdEvent::wakeup()
    {
        char buf[8] = {'a'};

        int rt = write(m_fd, buf, 8);  //写事件 会调用IO事件
        if (rt != 8)
        {
            ERRORLOG("write to wakeup fd less than 8 bytes , fd[%d]",m_fd);
        }
    }
}