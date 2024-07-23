#ifndef SOCK_TEST_UTIL_H
#define SOCK_TEST_UTIL_H
#include<sys/types.h>
#include<unistd.h>

namespace rocket{
    pid_t GetPid();
    pid_t GetThreadId();
}

#endif