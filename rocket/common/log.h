#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H
//宏定义 防止头文件重复

#include <string>
#include <queue>
#include <memory>
#include <semaphore.h>
#include "mutex.h"
#include "rocket/net/timer_event.h"

namespace rocket  
{

    // 作用:将字符串格式化为c风格的字符串
    template <typename... Args>   //可变参数模板
    std::string formatString(const char *str, Args &&...args)
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

//两部分组成 第一部分是logevent中的信息通过tostring实现 第二部分是传入的信息通过format函数实现
#define DEBUGLOG(str, ...)                                                                                                                                      \
    rocket::Logger::GetGlobalLogger()->pushLog((rocket::LogEvent(rocket::LogLevel::Debug)).toString() + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"+ rocket::formatString(str, ##__VA_ARGS__)+ "\n"); \

#define INFOLOG(str, ...)                                                                                                                                     \
    rocket::Logger::GetGlobalLogger()->pushLog((rocket::LogEvent(rocket::LogLevel::Info)).toString() + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"+ rocket::formatString(str, ##__VA_ARGS__ )+ "\n"); \

#define ERRORLOG(str, ...)                                                                                                                                     \
    rocket::Logger::GetGlobalLogger()->pushLog((rocket::LogEvent(rocket::LogLevel::Error)).toString() + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"+ rocket::formatString(str, ##__VA_ARGS__ )+ "\n"); \

#define APPDEBUGLOG(str, ...)                                                                                                                                     \
    rocket::Logger::GetGlobalLogger()->pushappLog((rocket::LogEvent(rocket::LogLevel::Debug)).toString() + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"+ rocket::formatString(str, ##__VA_ARGS__ )+ "\n"); \

    //枚举体
    enum LogLevel
    {
        Unknow = 0,
        Debug = 1,
        Info = 2,
        Error = 3,
    };

    // 根据日志级别获取对应级别的字符
    std::string LogLeveltoString(LogLevel);

    // 根据字符串转换为对应的日志级别
    LogLevel StringToLogLevel(const std::string &log_level);


class AsyncLogger
    {
    public:
        typedef std::shared_ptr<AsyncLogger> s_ptr;

        AsyncLogger(std::string file_name,std::string file_path, int max_file_size);

        static void* Loop(void*);

        void stop();

        //刷新搭配磁盘
        void flush();

        void PushLogBuffer(std::vector<std::string> &vec);

    private:

        std::queue<std::vector<std::string>>m_buffer;
        std::string m_file_path; // 日志输出路径
        std::string m_file_name; // 日志输出文件名字
        // 最终日志的路径为 m_file_path/m_file__yyyymmdd.1
        int m_max_file_size{0};

        sem_t m_semaphore;

        pthread_t m_thread;

        pthread_cond_t m_condtion;  //条件变量
        Mutex m_mutex;

        std::string m_date; //当前打印日志文件的文件日期
        FILE* m_file_handler {NULL}; //当前打开的日志文件的句柄

        bool m_reopen_flag{false}; //当前是否需要重新打开文件（日志重命名 当前日志文件打满了）

        int m_no{0};  //日志文件序号

        bool m_stop_flag{false};
    };

    // 日志器
    // 1.提供打印日志的方法
    // 2.设置日志输入的路径(文件/命令行)
    class Logger
    {
    public:
        void pushLog(const std::string &mes);
        void pushappLog(const std::string &mes);
        void log();
        static Logger *GetGlobalLogger();
        static void InitGlobalLogger(int type = 1);

        Logger(LogLevel level, int type = 1);

        void init(std::string m_file_path,std::string m_file_name,int max_size,int time_out);

        void syncLoop();

    private:
        LogLevel m_set_level;  //只有大于m_set_level才会打印
        std::vector<std::string> m_buffer;    //记录打印
        std::vector<std::string> m_app_buffer; 
        
        Mutex m_mutex;

        Mutex m_app_mutex;

        std::string m_file_path; //日志输出路径
        std::string m_file_name; //日志输出文件名字
        //最终日志的路径为 m_file_path/m_file__yyyymmdd.1
        int m_max_file_size{0};
                
        AsyncLogger::s_ptr m_asyn_logger; //异步日志器

        AsyncLogger::s_ptr m_asyn_app_logger; //异步日志器

        TimeEvent::s_ptr m_time_event;

        int m_type{1};

        int m_timeout{0};

    };

    // 日志时间类
    class LogEvent
    {
    public:
        LogEvent(LogLevel level) : m_level(level){};

        std::string getFileName() const
        {
            return m_file_name;
        }

        LogLevel getLogLevel() const
        {
            return m_level;
        }

        //格式化字符串
        std::string toString();

    private:
        std::string m_file_name; // 文件名
        std::string m_file_line; // 行号
        int m_pid;               // 进程号
        int m_thread_id;         // 线程号

        LogLevel m_level; // 日志级别

    };

}

#endif