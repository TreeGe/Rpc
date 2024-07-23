#include <pthread.h>
#include <assert.h>
#include "rocket/net/io_thread.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include "io_thread.h"

namespace rocket
{
    IOThread::IOThread()
    {
        int rt = sem_init(&m_init_semaphor,0,0);
        assert(rt==0);

        rt = sem_init(&m_start_semaphore, 0, 0);   //初始化信号量
        assert(rt==0);

        pthread_create(&m_thread, NULL, &IOThread::Main, this);  //创建一个子线程并绑定到Main函数

        //一直wait 直到新线程执行完Main函数的前置
        sem_wait(&m_init_semaphor);
        
        // printf("IOThread %d create success\n",m_thread_id);
        DEBUGLOG("IOThread %d create success",m_thread_id);
    }

    IOThread::~IOThread()
    {
        m_event_loop->stop();
        sem_destroy(&m_init_semaphor);
        sem_destroy(&m_start_semaphore);

        pthread_join(m_thread_id,NULL);   //主进程会等待线程执行结束  执行结束才会退出

        if(m_event_loop)
        {
            delete m_event_loop;
            m_event_loop = NULL;
        }
    }

    EventLoop *IOThread::getEventLoop()
    {
        return m_event_loop;
    }


    void *IOThread::Main(void *arg)
    {
        IOThread *thread = static_cast<IOThread *>(arg);

        thread->m_event_loop = new EventLoop();
        thread->m_thread_id = getThreadId();


        //唤醒等待的线程
        sem_post(&thread->m_init_semaphor);
        // thread->m_event_loop->loop();

        //让IO 线程等待  直到我们主动的启动  

        // printf("IOThread %d created,wait start semaphore\n",thread->m_thread_id);
        DEBUGLOG("IOThread %d created,wait start semaphore",thread->m_thread_id);

        sem_wait(&thread->m_start_semaphore);       //阻塞在这里
        // printf("IOthread %d start loop \n",thread->m_thread_id);
        DEBUGLOG("IOthread %d start loop",thread->m_thread_id);
        thread->getEventLoop()->loop();

        // printf("IOThread %d end loop \n",thread->m_thread_id);
        DEBUGLOG("IOThread %d end loop",thread->m_thread_id);

        return NULL;
    }

    void IOThread::start()
    {
        // printf("Now invoke IOThread %d",m_thread_id);
        INFOLOG("Now invoke IOThread %d",m_thread_id);
        sem_post(&m_start_semaphore);    //增加信号量
    }

    void IOThread::join()
    {
        pthread_join(m_thread,NULL);
    }
}