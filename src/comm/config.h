#ifndef SRC_COMM_CONFIG_H
#define SRC_COMM_CONFIG_H

#include <tinyxml/tinyxml.h> // 自己添加到/usr/lib的tinyxml库

namespace tinyrpc{

// 日志级别
enum LogLevel{
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    NONE = 5 // 不打印日志 
};

// 配置类
class Config{
public:
    typedef std::shared_ptr<Config> ptr;

public:
    // log params
    std::string m_log_path;  //日志路径
    std::string m_log_prefix; // 日志前缀
    int m_log_max_size {0}; // 一个日志文件最大的大小
    LogLevel m_log_level {LogLevel::DEBUG}; // 日志等级
    LogLevel m_app_log_level {LogLevel::DEBUG}; // app用户的日志等级
    int m_log_sync_inteval {500};  // log同步的间隔,ms

private:
    std::string m_file_path;

    TiXmlDocument* m_xml_file;
};

}


#endif
