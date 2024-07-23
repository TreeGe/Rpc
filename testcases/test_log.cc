#include "log.h"
#include <pthread.h>

void *fun(void *)
{
    int i=100;
    while(i-->0)
    {
        INFOLOG("this is thread in %s", "fun");
        INFOLOG("this is thread in %s", "fun");
    }
    return NULL;
}

int main()
{
    rocket::Logger::InitGlobalLogger();
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, &fun, NULL);
    if (ret != 0)
    {
        perror("pthread_create");
        return 1;
    }
    int i=100;
    while(i--)
    {
        DEBUGLOG("test log %s", "11");
    }

    pthread_join(thread,NULL);

    return 0;
}