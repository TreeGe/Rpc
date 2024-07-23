#ifndef ROCKET_TEST_LOG.H
#define ROCKET_TEST_LOG .H
#include <string.h>
#include <queue>
#include <memory>

namespace rocket
{
    // 日志级别设置
    enum LogLevel
    {
        DEBUG = 1,
        INFO = 2,
        ERROR = 3
    };

    // 设置日志事件
    class LogEvent
    {
    public:
        // 构造函数
        LogEvent(LogLevel loglevel) : m_loglevel(loglevel){};
        // 将所有信息转换为c语言风格的字符串
        std::string ToString();

    private:
        std::string m_file_name; // 文件名
        std::string m_file_line; // 所属行数
        LogLevel m_loglevel;     // 日志的级别
        int m_pid;               // 进程号
        int m_thread;            // 线程号
    };

    // 日志器  选择打印的方式
    class Logger
    {
    public:
        static Logger *GetGlobleLogger();
        void PushLog(const std::string &mes);
        void log();

    private:
        LogLevel m_set_loglevel;
        queue<std::string> m_buffer;
    };

    // 将输入的字符串转换为ｃ语言风格的字符串
    template <typename... Args>
    std::string formatstring(const char *str, Args &&...args)
    {
        int size = snprintf(nullptr, 0, str, args...);
        std::string result;
        if (size > 0)
        {
            result.resize(size);
            snprintf(&result[0], size + 1, str, args...);
        }
        return result;
    }

    // 将日志级别转换为zifuchuan
    std::string LogLevelToString(LogLevel)
    {
        switch (LogLevel)
        {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        default:
            return "ERROR";
        }
    }

#define DEBUG(str, ...)                                                 \
    std::string msg = (new rocket::LogEvent(rocket::LogLevel::DEBUG))->ToString() + rocket::formatstring(str, ##__VA_ARGS__);\
    rocket::Logger::GetGlobleLogger()->PushLog(msg);\
    msg = msg + "\n";\
    rocket::Logger::GetGlobalLogger()->Log();\
}

#endif ROCKET_TEST_LOG.H