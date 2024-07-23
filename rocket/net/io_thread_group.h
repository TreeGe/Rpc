// 本质是一个IO数组 或者 IO线程池
#ifndef ROCKET_NET_IOTHREAD_GROUP_H
#define ROCKET_NET_IOTHREAD_GROUP_H

#include <vector>
#include "rocket/common/log.h"
#include "rocket/net/io_thread.h"
namespace rocket
{
    class IOThreadGroup
    {
    public:
        IOThreadGroup(int size);

        ~IOThreadGroup();

        void start();

        void join();

        IOThread *getIOThread(); // 从当前线程组中获取一个可用的线程

    private:

        int m_size{0};                 //线程组的大小

        std::vector<IOThread *> m_io_thread_groups;

        int m_index{0};          //当前获取的Io线程的下表
    };
}

#endif