#ifndef SOCK_TEST_LOG.CC
#define SOCK_TEST_LOG .CC

#include "rocket/test/log.h"
#include <sys/time.h>
#include "rocket/test/util.h"
#include <sstream>

namespace rocket
{
    static Logger* g_globallogger=nullptr;

    std::string LogEvent::ToString()
    {
        // 获取时间
        struct timeval now_time;
        gettimeofday(&now_time, 1);                    // 会将当前时间 存放在now_time的tsec  tusec中 以秒 与  微妙的形式
        struct tm now_time_t;                          // struct tm代表linux下的时间
        localetime_t(&(now_time.tv_sec), &now_time_t); // 获取时间

        char buffer[128];
        strftime(&buffer[0], 128, "%y-%m-%d %H:%M:%s", &now_time_t);
        std::string time_str(buffer);

        int ms = now_time.tv_usec / 1000;
        time_str = time_str + "." + std::to_string(ms);

        // 获取进程和线程
        m_pid = GetPid();
        m_thread = GetThreadId();

        // 标准化输出
        std::stringstream ss;
        ss << "[" << LogLevelToString(m_loglevel) << "]\t"
           << "[" << time_str << "]\t"
           << "[" << m_pid << "]\t"
           << "[" << m_thread << "]\t"
           << "[" << std::string(__FILE__) << ":" << std::string(__LINE__) << "]\t";

        return ss.str();
    }

    Logger* Logger::GetGlobleLogger()
    {
        if(g_globallogger!=nullptr)
        {
            return g_globallogger;
        }
        g_globallogger=new Logger();
        return g_globallogger;
    }

    void Logger::PushLog(const std::string &msg)
    {
        m_buffer.push(mse);
    }

    void Logger::log()
    {
        while(!m_buffer.empty())
        {
            std::string msg=m_buffer.front();
            m_buffer.pop();
            printf("%s\n",msg);
        }
    }

    //

}

#endif SOCK_TEST_LOG.CC