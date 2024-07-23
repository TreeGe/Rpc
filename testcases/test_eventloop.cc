#ifndef ROCKET_TEST
#define ROCKET_TEST

#include "rocket/common/log.h"
#include "rocket/net/timer.h"
#include <pthread.h>
#include "rocket/net/fd_event.h"
#include "rocket/net/eventloop.h"
#include"rocket/net/io_thread.h"
#include"rocket/net/io_thread_group.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "string.h"

void test_io_thread()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        ERRORLOG("listenfd = -1");
        exit(0);
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_port = htons(12310);
    addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &addr.sin_addr);

    int rt = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    if (rt != 0)
    {
        ERRORLOG("bind error");
        exit(1);
    }
    rt = listen(listenfd, 5);
    if (rt != 0)
    {
        ERRORLOG("listen error");
        exit(0);
    }

    rocket::Fdevent event(listenfd);    //监听任务 只有当有新连接时才会打印
    event.listen(rocket::Fdevent::IN_EVENT, [listenfd]()
                 {
        sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        memset(&peer_addr,0,sizeof(peer_addr));
        int clientfd=accept(listenfd,reinterpret_cast<sockaddr*>(&peer_addr),&addr_len);
        if(clientfd==-1)
        {
            ERRORLOG("get clientfd failed !");
        }
        DEBUGLOG("success get client fd[%d], peer addr:  [%s:%d]",clientfd,inet_ntoa(peer_addr.sin_addr),ntohs(peer_addr.sin_port)); });

    int i = 0;
    rocket::TimeEvent::s_ptr timer_event = std::make_shared<rocket::TimeEvent>(
        1000, true, [&i]()
        { DEBUGLOG("trigger time event, count=%d", i++); });


    // rocket::IOThread io_thread;         //创建一个新线程  然后创建一个eventLoop事件

    // io_thread.getEventLoop()->addEpollEvent(&event);
    // io_thread.getEventLoop()->addTimerEvent(timer_event);
    // io_thread.start();
    // io_thread.join();

    rocket::IOThreadGroup io_thread_group(2);
    rocket::IOThread* io_thread= io_thread_group.getIOThread();
    io_thread->getEventLoop()->addEpollEvent(&event);
    io_thread->getEventLoop()->addTimerEvent(timer_event);

    rocket::IOThread* io_thread2= io_thread_group.getIOThread();
    io_thread2->getEventLoop()->addTimerEvent(timer_event);

    io_thread_group.start();
    io_thread_group.join();
}

int main()
{
    rocket::Logger::InitGlobalLogger();

    // rocket::EventLoop *eventloop = new rocket::EventLoop();

    test_io_thread();

    // int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // if (listenfd == -1)
    // {
    //     ERRORLOG("listenfd = -1");
    //     exit(0);
    // }
    // sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));

    // addr.sin_port = htons(12310);
    // addr.sin_family = AF_INET;
    // inet_aton("127.0.0.1", &addr.sin_addr);

    // int rt = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    // if (rt != 0)
    // {
    //     ERRORLOG("bind error");
    //     exit(1);
    // }
    // rt = listen(listenfd, 5);
    // if (rt != 0)
    // {
    //     ERRORLOG("listen error");
    //     exit(0);
    // }

    // rocket::Fdevent event(listenfd);
    // event.listen(rocket::Fdevent::IN_EVENT, [listenfd]()
    //              {
    //     sockaddr_in peer_addr;
    //     socklen_t addr_len = sizeof(peer_addr);
    //     memset(&peer_addr,0,sizeof(peer_addr));
    //     int clientfd=accept(listenfd,reinterpret_cast<sockaddr*>(&peer_addr),&addr_len);
    //     if(clientfd==-1)
    //     {
    //         ERRORLOG("get clientfd failed !");
    //     }
    //     DEBUGLOG("success get client fd[%d], peer addr:  [%s:%d]",clientfd,inet_ntoa(peer_addr.sin_addr),ntohs(peer_addr.sin_port)); });

    // int i = 0;
    // rocket::TimeEvent::s_ptr timer_event = std::make_shared<rocket::TimeEvent>(
    //     1000, true, [&i]()
    //     { printf("trigger time event, count=%d", i++); });

    // eventloop->addEpollEvent(&event);
    // eventloop->addTimerEvent(timer_event);

    // eventloop->loop();

    return 0;
}

#endif