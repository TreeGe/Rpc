#ifndef ROCKET_COMMON_RUN_TIME_H
#define ROCKET_COMMON_RUN_TIME_H

#include <string>

namespace rocket
{
    class RunTime
    {
    public:

    public:
        static RunTime *GetRunTime();


    public:
        std::string m_msgid;        //请求id
        std::string m_method_name;  //方法名
    };
}

#endif