#include <sys/time.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include "log.h"
#include "util.h"
#include"mutex.h"
#include "rocket/net/eventloop.h"
#include "rocket/common/runtime.h"

namespace rocket
{

    static Logger *g_logger = NULL;

    Logger::Logger(LogLevel level, int type) : m_set_level(level) ,m_type(type)
    {
        if (m_type == 2)
        {
            return;
        }
    }


    void Logger::syncLoop()
    {
        //同步m_buffer到sync_logger的buffer队尾
        std::vector<std::string> tmp_vec;
        ScopeMutex<Mutex>lock(m_mutex);
        m_buffer.swap(tmp_vec);
        lock.unlock();
        if(!tmp_vec.empty())
        {
            m_asyn_logger->PushLogBuffer(tmp_vec);
        }

        std::vector<std::string> tmp_vec2;
        ScopeMutex<Mutex> lock2(m_app_mutex);
        m_app_buffer.swap(tmp_vec2);
        lock2.unlock();
        if (!tmp_vec2.empty())
        {
            m_asyn_app_logger->PushLogBuffer(tmp_vec2);
        }
    }

    void Logger::init(std::string file_path, std::string file_name, int max_size, int timeout)
    {
        if (m_type == 2)
        {
            return;
        }
        m_file_path=file_path;
        m_file_name=file_name;
        m_max_file_size=max_size;
        m_timeout=timeout;

        m_time_event = std::make_shared<TimeEvent>(m_timeout, true, std::bind(&Logger::syncLoop, this));
        EventLoop::GetCurrentEventLoop()->addTimerEvent(m_time_event);
        m_asyn_logger = std::make_shared<AsyncLogger>(m_file_path, m_file_name + "_rpc", m_max_file_size); // TODO: 需要传入参数
        m_asyn_app_logger = std::make_shared<AsyncLogger>(m_file_path, m_file_name + "_app", m_max_file_size);
    }

    Logger *Logger::GetGlobalLogger()
    {
        return g_logger;
    }

    void Logger::InitGlobalLogger(int type)
    {
        printf("Init log level [DEBUG] \n");
        g_logger=new Logger(Debug,type);
        g_logger->init("../log/", "test_rpc_server", 1000000, 1000);
    }

    std::string LogLeveltoString(LogLevel level)
    {
        switch (level)
        {
        case Debug:
            return "Debug";
        case Info:
            return "Info";
        case Error:
            return "Error";
        default:
            return "Unknow";
        }
    }

    LogLevel StringToLogLevel(const std::string &log_level)
    {
        if(log_level=="Debug")
        {
            return Debug;
        }else if(log_level=="Info")
        {
            return Info;
        }else if(log_level=="Error")
        {
            return Error;
        }else
        {
            return Unknow;
        }
    }

    // 封装时间  标准化输出
    std::string LogEvent::toString()
    {
        struct timeval now_time;

        gettimeofday(&now_time, nullptr);  //得到的是距离某个时间的秒数

        struct tm now_time_t;
        localtime_r(&(now_time.tv_sec), &now_time_t);  //中间体

        char buf[128];
        strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);  //格式化
        std::string time_str(buf);

        int ms = now_time.tv_usec / 1000;
        time_str = time_str + "." + std::to_string(ms);

        // 获取进程和线程
        m_pid = getPid();
        m_thread_id = getThreadId();

        // 获取文件名和行号
        std::stringstream ss;
        ss << "[" << LogLeveltoString(m_level) << "]\t"
           << "[" << time_str << "]\t"
           << "[" << m_pid << ":" << m_thread_id << "]\t";

        std::string msgid = RunTime::GetRunTime()->m_msgid;
        std::string method_name = RunTime::GetRunTime()->m_method_name;
        if(!msgid.empty())
        {
            ss << "[" << msgid << "]\t";
        }
        if (!method_name.empty())
        {
            ss << "[" << method_name << "]\t";
        }

        return ss.str();
    }

    //向缓冲区写数据的时候  需要上锁 解锁
    void Logger::pushLog(const std::string &msg)
    {
        if(m_type==2)
        {
            printf("msg%s\n",msg.c_str());
        }
        // printf("pushLog\n");
        ScopeMutex<Mutex> lock(m_mutex);
        m_buffer.push_back(msg);
        //lock.unlock();
    }
    void Logger::pushappLog(const std::string &msg)
    {
        ScopeMutex<Mutex> lock(m_app_mutex);
        m_app_buffer.push_back(msg);
    }

    //向屏幕打印日志的时候  需要上锁 解锁  单例对象
    void Logger::log()
    {
        // ScopeMutex<Mutex>lock(m_mutex);
        // while (!m_buffer.empty())
        // {
        //     std::string msg = m_buffer.front();
        //     m_buffer.pop();
        //     printf(msg.c_str());
        // }
    }

    AsyncLogger::AsyncLogger(std::string file_path,std::string file_name,int max_file_size) 
        : m_file_path(file_path),m_file_name(file_name),m_max_file_size(max_file_size)
    {
        sem_init(&m_semaphore,0,0);

        pthread_create(&m_thread,NULL,&AsyncLogger::Loop,this);

        // pthread_cond_init(&m_condtion, NULL);

        sem_wait(&m_semaphore);
    }

    void *AsyncLogger::Loop(void *arg)
    {
        // 将buffer里面的数据全部打印到文件中  然后线程睡眠直到有新的数据再重复这个过程

        AsyncLogger *logger = reinterpret_cast<AsyncLogger*>(arg);

        pthread_cond_init(&logger->m_condtion, NULL);

        sem_post(&logger->m_semaphore);

        while (1)
        {
            ScopeMutex<Mutex> lock(logger->m_mutex);
            while (logger->m_buffer.empty())
            {
                pthread_cond_wait(&(logger->m_condtion), logger->m_mutex.getmutex());
            }
            printf("pthread_cond_wait back \n");

            std::vector<std::string> tmp;
            // logger->m_buffer.front().swap(tmp);
            tmp.swap(logger->m_buffer.front());
            logger->m_buffer.pop();

            lock.unlock();

            timeval now;
            gettimeofday(&now, NULL);

            struct tm now_time;
            localtime_r(&(now.tv_sec), &now_time);

            const char* format = "%Y%m%d";
            char date[32];
            strftime(date, sizeof(date), format, &now_time);

            if (std::string(date) != logger->m_date)
            {
                logger->m_no = 0;
                logger->m_reopen_flag = true;
                logger->m_date = std::string(date);
            }
            if (logger->m_file_handler == NULL)
            {
                logger->m_reopen_flag = true;
            }

            std::stringstream ss;
            printf("logger->m_file_path%s\n",logger->m_file_path.c_str());
            printf("logger->m_file_name%s\n",logger->m_file_name.c_str());
            ss << logger->m_file_path << logger->m_file_name << "_" << std::string(date) << "_log.";
            std::string log_file_name = ss.str() + std::to_string(logger->m_no);
            if (logger->m_reopen_flag)
            {
                if (logger->m_file_handler)
                {
                    fclose(logger->m_file_handler);
                }
                printf("log_file_name = %s \n",log_file_name.c_str());
                logger->m_file_handler = fopen(log_file_name.c_str(), "a");
                if(logger->m_file_handler==NULL)
                {
                    printf("open file failed\n");
                }
                logger->m_reopen_flag = false;
            }
            if (ftell(logger->m_file_handler) > logger->m_max_file_size)
            {
                fclose(logger->m_file_handler);
                logger->m_no++;
                log_file_name = ss.str() + std::to_string(logger->m_no++);
                logger->m_file_handler = fopen(log_file_name.c_str(), "a");
                logger->m_reopen_flag = false;
            }

            for (auto &i : tmp)
            {
                if (!i.empty())
                {
                    fwrite(i.c_str(), 1, i.length(), logger->m_file_handler);
                }
            }
            fflush(logger->m_file_handler);

            if(logger->m_stop_flag)
            {
                return NULL;
            }
        }

        return NULL;
    }

    void AsyncLogger::stop()
    {
        m_stop_flag = true;
    }

    void AsyncLogger::flush()
    {
        if(m_file_handler)
        {
            fflush(m_file_handler);
        }
    }

    void AsyncLogger::PushLogBuffer(std::vector<std::string> &vec)
    {
        ScopeMutex<Mutex> lock(m_mutex);
        m_buffer.push(vec);

        //这个时候需要唤醒异步日志线程
        pthread_cond_signal(&m_condtion);
        lock.unlock();

        //这时候需要唤醒异步日志线程
        printf("pthread_cond_signal \n");
    }
}