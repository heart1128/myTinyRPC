#include <sys/syscall.h> // syscall系统调用
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h> // gettimeofday()
#include <signal.h>


#include "src/comm/log.h"
#include "src/comm/config.h"
#include "src/coroutine/coroutine.h"
#include "src/comm/run_time.h"

namespace tinyrpc{

tinyrpc::Logger::ptr gRpcLogger;
tinyrpc::Config::ptr gRpcConfig;

// coredump处理函数
void CoredumpHandler(int signal_no)
{
    // errorLog返回了一个stringstream流
    ErrorLog << "progress received invalid signal, will exit";
    printf("progress received invalid signal, will exit\n");

    gRpcLogger->flush();
    // 最后的coredump把剩余的日志加入线程写入，是整个队列写入。m_thread是在初始化每个asyncLogger对象创建的
    pthread_join(gRpcLogger->getAsyncLogger()->m_thread, NULL);
    pthread_join(gRpcLogger->getAsyncAppLogger()->m_thread, NULL);
}
// thread_local c++11关键字
// 这个变量值在线程周期内存活,保证线程有自己的变量
static thread_local pid_t t_thread_id = 0;
static pid_t g_pid = 0;

// --------------- 辅助函数
// 获取线程id
pid_t gettid()
{
    if(t_thread_id == 0)
        t_thread_id = syscall(SYS_gettid); // 系统信号调用，返回线程id

    return t_thread_id;
}
// 打开日志
bool OpenLog()
{
    // 日志对象没有实例化，表示日志关闭
    if(!gRpcLogger)
        return false;

    return true;
}

// 日志等级enum转化为string
std::string levelToString(LogLevel level)
{
    /*
    enum LogLevel{
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    NONE = 5 // 不打印日志 
    */
    std::string re = "DEBUG"; // 默认DeBug级别的日志
    switch(level)
    {
        case DEBUG:
            re = "DEBUG";
            break;
        case INFO:
            re = "INFO";
        case WARN:
            re = "WARN";
        case ERROR:
            re = "ERROR";
        case NONE:
            re = "NONE";
        default:
            break;
    }

    return re;
}

// string还原成enum
LogLevel stringToLevel(const std::string& str)
{
    LogLevel level = LogLevel::DEBUG;

    if(str == "DEBUG")
        level = LogLevel::DEBUG;
    else if(str == "INFO")
        level = LogLevel::INFO;
    else if(str == "WARN")
        level = LogLevel::WARN;
    else if(str == "ERROR")
        level = LogLevel::ERROR;
    else if(str == "NONE")
        level = LogLevel::NONE;
    
    return level;
}

// 日志类型转换为string
std::string logTypeToString(LogType type)
{
    switch (type)
    {
    case APP_LOG:
        return "app";
    case RPC_LOG:
        return "rpc";
    default:
        return "";
    }
}
/*


************** LogTmp成员函数 **************


*/ 



LogTmp::LogTmp(LogEvent::ptr event)
: m_event(event)
{

}
LogTmp::~LogTmp()
{
    m_event->log();
}

std::stringstream& LogTmp::getStringStream()
{
    return m_event->getStringStream();
}


/*


************** LogEvent成员函数 **************


*/ 

 
// logEvent构造
LogEvent::LogEvent(LogLevel level, const char* file_name, int line, const char* func_name, LogType type)
    :m_level(level),
     m_fileName(file_name),
     m_line(line),
     m_funcName(func_name),
     m_type(type)
{}
// 析构
LogEvent::~LogEvent()
{}

// 1. 把日志信息传输到成员m_ss，一个stringstream流对象
std::stringstream& LogEvent::getStringStream()
{
    // 1.1 获取时间戳，计算从1970年1月1号00:00（UTC）到当前的时间跨度,头文件<time.h>
    gettimeofday(&m_timeVal, nullptr);

    struct tm time;
    localtime_r(&(m_timeVal.tv_sec), &time);  // 时间戳放在tv_sec中。_r是线程安全可重入函数

    // 1.2 格式化时间
    const char* format = "%Y-%m-%d %H:%M:%S";
    char buf[128];
    strftime(buf, sizeof(buf), format, &time); // 作用就是指定时间格式，把time转换为字符串存到buf中

    // 1.3 日志信息写入流
        // 1.3.1 写入时间 [xxxx-xx-xx xx:xx:xx]
    m_ss << "[" << buf << "." << m_timeVal.tv_usec << "]\t";
        // 1.3.2 写入日志等级
    std::string s_level = levelToString(m_level);
    m_ss << "[" << s_level << "]\t";
        // 1.3.3 写入文件名，线程id，进程id，行号
    if(g_pid == 0)
        g_pid = getpid();  // 是父进程
    
    m_pid = g_pid;

    if(t_thread_id == 0)
        t_thread_id = gettid(); 
    
    m_tid = t_thread_id;

    // 协程id
    m_cor_id = Coroutine::getCurrentCoroutine()->getCorId();

    m_ss << "[" << m_pid  << "]\t"
        << "[" << m_tid << "]\t"
        << "[" << m_cor_id << "]\t"
        << "[" << m_fileName << ":" << m_line << "]\t";

    // 协程runtime
    RunTime* runtime = getCurrentRunTime();
    if(runtime)
    {
        std::string msgno = runtime->m_msg_no;
        if(!msgno.empty())
            m_ss << "[" << msgno << "]\t";
        
        std::string interface_name = runtime->m_interface_name;
        if(!interface_name.empty())
            m_ss << "[" << interface_name << "]\t";
    }

    return m_ss;
}

// stringstream流转string
std::string LogEvent::toString()
{
    return getStringStream().str();
}

// 日志事件启动加入用户buffer
void LogEvent::log()
{
    m_ss << "\n";
    // 如果当前日志的等级大于需要写入的等级并且对应相应的PRC or APP类型，就对应写入。
    if(m_level >= gRpcConfig->m_log_level && m_type == RPC_LOG)
        gRpcLogger->pushRpcLog(m_ss.str());
    else if(m_level >= gRpcConfig->m_app_log_level && m_type == APP_LOG)
        gRpcLogger->pushAppLog(m_ss.str());
}



/*


************** Logger成员函数 **************

*/ 

Logger::Logger(){}
Logger::~Logger()
{
    flush();
    // 加入线程进行异步写入
    pthread_join(m_async_rpc_logger->m_thread, NULL);
    pthread_join(m_async_app_logger->m_thread, NULL);
}

// 获取日志对象
Logger* Logger::getLogger()
{
    return gRpcLogger.get(); // 智能指针，获取的是const ptr
}

// 初始化日志
void Logger::init(const char* file_name, const char* file_path, int max_size, int sync_inteval)
{
    // 判断是否日志初始化
    if(!m_is_init)
    {
        m_sync_inteval = sync_inteval;
        // 初始化100万个buffer
        for(int i = 0 ; i < 1000000; ++i)
        {
            m_buffer.emplace_back("");
            m_app_buffer.emplace_back("");
        }

        // 指针初始化
        m_async_rpc_logger = std::make_shared<AsyncLogger>(file_name, file_path, max_size, RPC_LOG);
        m_async_app_logger = std::make_shared<AsyncLogger>(file_name, file_path, max_size, APP_LOG);

        // 出现以下信号表示突然程序中断，这时要保存中断的日志，需要一个Coredump处理函数
        signal(SIGSEGV, CoredumpHandler);   // 建立core文件，段非法错误
        signal(SIGABRT, CoredumpHandler);   // abort
        signal(SIGTERM, CoredumpHandler);   // 进程终止 
        signal(SIGKILL, CoredumpHandler);   // 杀死进程
        signal(SIGINT, CoredumpHandler);    // ctrl + c
        signal(SIGSTKFLT, CoredumpHandler);  // stack fault
        // 忽略pipe信号，比如对没有读进程的管道写
        signal(SIGPIPE, SIG_IGN);
        
        m_is_init = true;
    }
}

// 启动日志
void Logger::start()
{
    // 框架搭建好进行启动
}


// 把buffer替换出来，然后重新加入任务
void Logger::loopFunc()
{
    std::vector<std::string> app_tmp;
    // 上app buffer锁
    Mutex::Lock lock1(m_app_buffer_mutex);
    app_tmp.swap(m_app_buffer);
    // 解锁
    lock1.unlock();

    std::vector<std::string> tmp;
    // 上buffer锁
    Mutex::Lock lock2(m_buffer_mutex);
    tmp.swap(m_buffer);
    // 解锁
    lock2.unlock();

    // 加入日志任务AsyncLogger类的task
    m_async_rpc_logger->push(tmp);
    m_async_app_logger->push(app_tmp);
}


// 写入PRC类型日志
void Logger::pushRpcLog(const std::string& msg)
{
    // 上锁
    Mutex::Lock lock(m_buffer_mutex);
    m_buffer.emplace_back(std::move(msg)); // push_back(std::move(msg))一样都是移动拷贝构造T&&
    // 解锁
    lock.unlock();
}

// 写入APP类型日志
void Logger::pushAppLog(const std::string& msg)
{
    // 上锁 防止多个协程程同时写入，就乱了
    Mutex::Lock lock(m_app_buffer_mutex);
    m_app_buffer.emplace_back(std::move(msg));
    // 解锁
    lock.unlock();
}

// 刷新
void Logger::flush()
{
    // 置换缓冲区
    loopFunc();
    // 刷新
    m_async_rpc_logger->stop();
    m_async_rpc_logger->flush();

    m_async_app_logger->stop();
    m_async_app_logger->flush();
    
}



/*


************** AsyncLogger成员函数 **************

*/ 
AsyncLogger::AsyncLogger(const char* file_name, const char* file_path, int max_size, LogType logType)
: m_fileName(file_name),
  m_filePath(file_path),
  m_maxSize(max_size),
  m_logType(logType)
{
    // 初始化信号量
    /*
    extern int sem_init __P ((sem_t *__sem, int __pshared, unsigned int __value));　　
    sem为指向信号量结构的一个指针；
    pshared不为０时此信号量在进程间共享，否则只能为当前进程的所有线程共享；
    value给出了信号量的初始值。　
    */
    int rt = sem_init(&m_semaphore, 0, 0);
    assert(rt == 0);

    // 创建异步日志线程
    // 保存tid的变量，内存分配策略一般为空，执行的函数，传递的参数
    rt = pthread_create(&m_thread, nullptr, &AsyncLogger::exeute, this);
    assert(rt == 0);

    // 信号量-1，如果信号量为0则阻塞等待信号量不为0      P操作
    rt = sem_wait(&m_semaphore);
    assert(rt == 0);

}
AsyncLogger::~AsyncLogger()
{

}

void* AsyncLogger::exeute(void* arg)
{
    /*
    1. 取任务队列的一个日志vector<string>
    2. 打开文件写入
    3. 限制单个文件的大小，超过就把编号++，写入新的文件。
    */

    // 为什么类指针转换不用static_cast<>
    AsyncLogger* ptr = reinterpret_cast<AsyncLogger*>(arg);

    // 初始化pthread_cond_t 的条件变量
    int rt = pthread_cond_init(&ptr->m_condition, NULL);
    assert(rt == 0);

    rt = sem_post(&ptr->m_semaphore); // V操作
    assert(rt == 0);

    // 循环执行
    while(true)
    {
        // 每次拿任务信息是原子的，需要上锁，
        Mutex::Lock lock(ptr->m_mutex);

        // 任务队列是空的，并且还没有暂停写日志，就等待条件变量wait阻塞，并且需要上锁
        while(ptr->m_tasks.empty() && !ptr->m_stop)
        {
            pthread_cond_wait(&(ptr->m_condition), ptr->m_mutex.getMutex());
        }

        std::vector<std::string> tmp;
        tmp.swap(ptr->m_tasks.front());
        ptr->m_tasks.pop();
        bool is_stop = ptr->m_stop;
        
        // 解锁
        lock.unlock();


        // 获取当前时间和时间戳
        timeval now;
        gettimeofday(&now, nullptr);

        struct tm now_time;
        localtime_r(&(now.tv_sec), &now_time);

        const char* format = "%Y%m%d";
        char date[32];
            // 格式化时间
        strftime(date, sizeof(date), format, &now_time);
        if(ptr->m_date != std::string(date))  // 保存的时间不对等，换最新获取的时间
        {
            // cross day
            // reset m_no m_date
            ptr->m_no = 0;
            ptr->m_date = std::string(date);
            ptr->m_need_reopen = true;  // 时间改了要打开过文件
        }

        if(!ptr->m_fileHandle)  // 文件没有打开初始化
        {
            ptr->m_need_reopen = true;
        }

        std::stringstream ss;  // 日志文件名
        ss << ptr->m_filePath << ptr->m_fileName << "_" << ptr->m_date << "_" << logTypeToString(ptr->m_logType)
            << "_" << ptr->m_no << ".log";
        std::string full_file_name = ss.str();

            // 文件需要重新打开
        if(ptr->m_need_reopen)
        {
            if(ptr->m_fileHandle)
                fclose(ptr->m_fileHandle);

            // 重新打开, 附加形式, 如果打开出错错误自动保存在errno
            ptr->m_fileHandle = fopen(full_file_name.c_str(), "a");
            if(!ptr->m_fileHandle)
                printf("file %s open fail errno = %d reason = %s \n",full_file_name.c_str(), errno, strerror(errno));

            ptr->m_need_reopen = false;
        }

        // 当前文件流的位置大于设置的单个日志最大值，处理到下一个文件中。
        if(ftell(ptr->m_fileHandle) > ptr->m_maxSize)
        {
            fclose(ptr->m_fileHandle);

            // 1. 单个日志超过最大值
            ptr->m_no++; // 编号++
            std::stringstream ss2;  // 因为编号变了，所以需要写过文件名
            ss2 << ptr->m_filePath << ptr->m_fileName << "_" << ptr->m_date << "_" << logTypeToString(ptr->m_logType)
                << "_" << ptr->m_no << ".log";
            full_file_name = ss2.str();
            // 2. 创建打开一个新文件
            ptr->m_fileHandle = fopen(full_file_name.c_str(), "a");
            ptr->m_need_reopen = false;
        }
            // 总体判断下文件是否打开
        if(!ptr->m_fileHandle)
            printf("file %s open fail errno = %d reason = %s \n",full_file_name.c_str(), errno, strerror(errno));


        // tmp是一个用户的vector<string>日志向量
        for(auto str : tmp)
        {
            // 日志写入文件
            if(!str.empty()) 
                // char[], sizeof(char), size , FILE*
                fwrite(str.c_str(), 1, str.size(), ptr->m_fileHandle);
        }
        tmp.clear();
        fflush(ptr->m_fileHandle);
        if(is_stop)  // 暂停了就跳出while(true)
            break;
    }

    if(!ptr->m_fileHandle)
        fclose(ptr->m_fileHandle);

    return nullptr;
}

// 异步日志加入队列
void AsyncLogger::push(std::vector<std::string>& buffer)
{
    if(!buffer.empty())
    {
        // 上锁
        Mutex::Lock lock(m_mutex);
        m_tasks.push(buffer);
        // 解锁
        lock.unlock();
        // 唤醒等待的条件变量，也就是执行函数里面的等待的条件变量。
        pthread_cond_signal(&m_condition);
    }
}

// 刷新文件指针
void AsyncLogger::flush()
{
    if(m_fileHandle)
        fflush(m_fileHandle);
}

// 暂停写入
void AsyncLogger::stop()
{
    if(!m_stop)
    {
        m_stop = true;
        // 暂停了，但是等待的条件变量不能阻塞
        pthread_cond_signal(&m_condition);
    }
}

void Exit(int code)
{
    printf("It's sorry to said we start TinyRPC server error, look up log file to get more deatils!\n");
    gRpcLogger->flush();
    pthread_join(gRpcLogger->getAsyncLogger()->m_thread, NULL);
}

};