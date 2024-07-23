#ifndef ROCKET_COMMON_UTIL_CC
#define ROCKET_COMMON_UTIL_CC


#include<sys/types.h>
#include<unistd.h>
#include<sys/time.h>
#include<sys/syscall.h>
#include<string.h>
#include <arpa/inet.h>
#include"util.h"

namespace rocket{

    static int g_pid=0;    //进程是全局变量 因为一个加程序只有一个进程

    static thread_local int t_thread_id = 0;  //线程是私有变量 因为一个进程内会有多个线程

    pid_t getPid(){
        if(g_pid!=0){
            return g_pid;    
        }
        return getpid();
    }

    pid_t getThreadId(){
        if(t_thread_id!=0){
            return t_thread_id;
        }
        return syscall(SYS_gettid);
    }

    int64_t getNowMs()
    {
        timeval val;
        gettimeofday(&val,NULL);
        return val.tv_sec*1000+val.tv_usec/1000;
    }

    int32_t getInt32FromNetByte(const char* buff)
    {
        int32_t re;
        memcpy(&re,buff,sizeof(re));
        return ntohl(re);
    }
}

#endif