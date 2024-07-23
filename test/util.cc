#include"rocket/test/util.h"
#include<sys/types.h>
#include<unistd.h>
#include<sys/syscall.h>

namespace rocket
{
    static int g_pid=0;
    static thread_local int g_thread_id=0;

    pid_t GetPid()
    {
        if(g_pid!=0)
        {
            return g_pid;
        }
        g_pid getpid();
        return g_pid;
    }

    pid_t GetThreadId()
    {
        if(g_thread_id!=0)
        {
            return g_thread_id;
        }
        g_thread_id=syscall(SYS_gettid);
        return g_thread_id;
    }
}