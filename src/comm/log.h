#ifndef SRC_COMM_LOG_H
#define SRC_COMM_LOG_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <string>
#include <cstdio>
#include <vector>
#include <queue>

#include <sstream> // stringstream
#include <memory> // shared_ptr


#include "src/comm/config.h"
#include "src/net/mutex.h"
/*
class Logger : 处理单个用户的日志string写入buffer(vector)
class AsyncLogger : 处理整个日志队列的写入文件，每次处理一个用户的日志
class LoggerEvent : 被Logger调用，实现日志字符的组织，日志等级等等。
*/

namespace tinyrpc{

// 其他函数要用日志，就要有gRpcConfig
extern tinyrpc::Config::ptr gRpcConfig;

// 格式化字符串函数，使用可变模版参数
template<typename... Args>
std::string formatString(const char* str, Args&&... args)
{
    // snprintf会限制size，sprintf不会限制
    int size = snprintf(nullptr, 0, str, args...);

    std::string result;
    // 先看有没有要写入的，有写入的在进行字符串格式化
    if(size > 0)
    {
        result.resize(size);
        snprintf(&result[0], size + 1, str, args...);
    }
    return result;
}

// 日志类型
enum LogType{
    RPC_LOG = 1, // RPC框架Debug日志
    APP_LOG = 2 // 用户级日志
};
/*

--------------RPC LOG加入buffer

*/
// 使用LogEvent对象进行日志写入stringstram流，之后LogTmp析构之后加入buffer
#define DebugLog \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::DEBUG >= tinyrpc::gRpcConfig->m_log_level) \
        tinyrpc::LogTmp(tinyrpc::LogEvent::ptr(new tinyrpc::LogEvent(tinyrpc::LogLevel::DEBUG, __FILE__, __LINE__, __func__, \
        tinyrpc::LogType::RPC_LOG))).getStringStream()

#define InfoLog \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::INFO >= tinyrpc::gRpcConfig->m_log_level) \
        tinyrpc::LogTmp(tinyrpc::LogEvent::ptr(new tinyrpc::LogEvent(tinyrpc::LogLevel::INFO, __FILE__, __LINE__, __func__, \
        tinyrpc::LogType::RPC_LOG))).getStringStream()

#define WarnLog \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::WARN >= tinyrpc::gRpcConfig->m_log_level) \
        tinyrpc::LogTmp(tinyrpc::LogEvent::ptr(new tinyrpc::LogEvent(tinyrpc::LogLevel::WARN, __FILE__, __LINE__, __func__, \
        tinyrpc::LogType::RPC_LOG))).getStringStream()

#define ErrorLog \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::ERROR >= tinyrpc::gRpcConfig->m_log_level) \
        tinyrpc::LogTmp(tinyrpc::LogEvent::ptr(new tinyrpc::LogEvent(tinyrpc::LogLevel::ERROR, __FILE__, __LINE__, __func__, \
        tinyrpc::LogType::RPC_LOG))).getStringStream()

/*

--------------APP LOG加入buffer

宏定义 ... 和 ##__VA_ARGS__配套使用，可变参数，之后传入可变模版参数进行解析
*/

#define AppDebugLog(str, ...) \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::DEBUG >= tinyrpc::gRpcConfig->m_app_log_level) \
    { \
       tinyrpc::Logger::getLogger()->pushAppLog(tinyrpc::LogEvent(tinyrpc::LogLevel::DEBUG, __FILE__, __LINE__, __func__, tinyrpc::LogType::APP_LOG).toString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + tinyrpc::formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define AppInfoLog(str, ...) \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::INFO >= tinyrpc::gRpcConfig->m_app_log_level) \
    { \
       tinyrpc::Logger::getLogger()->pushAppLog(tinyrpc::LogEvent(tinyrpc::LogLevel::INFO, __FILE__, __LINE__, __func__, tinyrpc::LogType::APP_LOG).toString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + tinyrpc::formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define AppWarnLog(str, ...) \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::WARN >= tinyrpc::gRpcConfig->m_app_log_level) \
    { \
       tinyrpc::Logger::getLogger()->pushAppLog(tinyrpc::LogEvent(tinyrpc::LogLevel::WARN, __FILE__, __LINE__, __func__, tinyrpc::LogType::APP_LOG).toString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + tinyrpc::formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define AppErrorLog(str, ...) \
    if(tinyrpc::OpenLog() && tinyrpc::LogLevel::ERROR >= tinyrpc::gRpcConfig->m_app_log_level) \
    { \
       tinyrpc::Logger::getLogger()->pushAppLog(tinyrpc::LogEvent(tinyrpc::LogLevel::ERROR, __FILE__, __LINE__, __func__, tinyrpc::LogType::APP_LOG).toString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + tinyrpc::formatString(str, ##__VA_ARGS__) + "\n"); \
    } \


/*

--------------辅助函数

*/
// 日志打印获取prd
pid_t gettid();
// string to enumLevel
LogLevel stringToLevel(const std::string& str);
std::string levelToString(LogLevel level);

std::string logTypeToString(LogType type);
bool OpenLog();


// *********************日志打印辅助类*********************
class LogTmp{
public:
    explicit LogTmp(LogEvent::ptr event);
    ~LogTmp();

    std::stringstream& getStringStream();

private:
    LogEvent::ptr m_event;
};


// *********************日志事件类*********************
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(LogLevel level, const char* file_name, int line, const char* func_name, LogType type);

    ~LogEvent();

    std::string toString();
    // 日志都放在stringstream内，最后进行拿出来打印
    std::stringstream& getStringStream();

    void log();
    

private:
    timeval m_timeVal; // 日志时间 ,时间结构体，m和ms
    LogLevel m_level; // 日志等级
    pid_t m_pid{0};  // 进程id
    pid_t m_tid {0}; // 线程id
    int m_cor_id{0};

    const char* m_fileName; // 打印的文件名
    int m_line{0};   // 打印的行数
    const char* m_funcName; // 日志打印的函数名
    LogType m_type; // 日志类型
    std::string m_msgNo; // 消息体

    std::stringstream m_ss; // 存储整个日志的流
    std::stringstream m_ss;
};



// *********************异步日志类*********************
class AsyncLogger{
public:
    typedef std::shared_ptr<AsyncLogger> ptr;

    AsyncLogger(const char* file_name, const char* file_path, int max_size, LogType logType);
    ~AsyncLogger();

    // 传入本次日志的buffer，加入到异步队列中等待打印
    void push(std::vector<std::string>& buffer);
    // 刷新buffer
    void flush();
    // 开始异步执行
    static void* exeute(void*);

    void stop();

public:
    // 每个用户都使用一个vector每个str日志
    std::queue<std::vector<std::string>> m_tasks;  // 异步日志队列

private:
    const char* m_fileName;
    const char* m_filePath;
    int m_maxSize{0};   // 单个日志文件的最大写入数，限定一个日志文件的长度，分成多个日志文件
    LogType m_logType;
    int m_no{0};  // 标识当前的日志文件的编号，也就是第几个日志文件
    bool m_need_reopen {false};   // 是否需要重新打开，更新数据等等会重新打开文件， fileHandle没有初始化也会
    FILE* m_fileHandle {nullptr};
    std::string m_date;

    Mutex m_mutex;  // 异步锁
    pthread_cond_t m_condition; // 条件变量
    bool m_stop {false};

public:
    pthread_t m_thread; // 异步使用一个线程进行异步打印
    sem_t m_semaphore;  // 信号量
};


// *********************日志类*********************
class Logger{
public:
    static Logger* getLogger();

public:
    typedef std::shared_ptr<Logger> ptr;

    Logger();
    ~Logger();

    void init(const char* file_name, const char* file_path, int max_size, int sync_inteval);
    // 加入不同的日志类型。
    void pushRpcLog(const std::string& log_msg);
    void pushAppLog(const std::string& log_msg);
    void loopFunc();

    void flush();
    void start();

public:
    AsyncLogger::ptr getAsyncLogger()
    {
        return m_async_rpc_logger;
    }
    AsyncLogger::ptr getAsyncAppLogger()
    {
        return m_async_app_logger;
    }

public:
    std::vector<std::string> m_buffer;    // rpc日志buffer
    std::vector<std::string> m_app_buffer; // app日志buffer

private:
    Mutex m_app_buffer_mutex;
    Mutex m_buffer_mutex;

    bool m_is_init {false};
    AsyncLogger::ptr m_async_rpc_logger;
    AsyncLogger::ptr m_async_app_logger;

    int m_sync_inteval {0}; // 同步间隔时间
};


void Exit(int code);

}

#endif