#include <assert.h>
#include <stdio.h>
#include <memory>
#include <algorithm>
#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/net/tcp/tcp_server.h"
#include "src/net/net_address.h"
#include <tinyxml/tinyxml.h>
#include "config.h"

// 一个xml保存了Log和db配置

namespace tinyrpc {

extern tinyrpc::Logger::ptr gRpcLogger;
extern tinyrpc::TcpServer::ptr gRpcServer;


Config::Config(const char *file_path)
: m_file_path(file_path)
{   
    // 创建xml类，读取xml文件
    m_xml_file = new TiXmlDocument();
    bool rt = m_xml_file->LoadFile(file_path);
    if(!rt)
    {
        printf("start tinyrpc server error! read conf file [%s] error info: [%s], errorid: [%d], error_row_column:[%d row %d column]\n", 
        file_path, m_xml_file->ErrorDesc(), m_xml_file->ErrorId(), m_xml_file->ErrorRow(), m_xml_file->ErrorCol());
        exit(0);
    }

}

Config::~Config()
{
    if(m_xml_file)
    {
        delete m_xml_file;
        m_xml_file = NULL;
    }
}
// 读取整个rpc的配置
void Config::readConf()
{
    // root
    TiXmlElement* root = m_xml_file->RootElement();
    TiXmlElement* log_node = root->FirstChildElement("log");
    if (!log_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [log] xml node\n", m_file_path.c_str());
        exit(0);
    }

    // 解析log配置
    readLogConfig(log_node);

    // 协程配置 ： stack_size 和 coroutine_pool_size
    TiXmlElement* coroutine_node = root->FirstChildElement("coroutine");
    if (!coroutine_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [coroutine] xml node\n", m_file_path.c_str());
        exit(0);
    }

    if (!coroutine_node->FirstChildElement("coroutine_stack_size") || !coroutine_node->FirstChildElement("coroutine_stack_size")->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [coroutine.coroutine_stack_size] xml node\n", m_file_path.c_str());
        exit(0);
    }

    if (!coroutine_node->FirstChildElement("coroutine_pool_size") || !coroutine_node->FirstChildElement("coroutine_pool_size")->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [coroutine.coroutine_pool_size] xml node\n", m_file_path.c_str());
        exit(0);
    }

    int cor_stack_size = std::atoi(coroutine_node->FirstChildElement("coroutine_stack_size")->GetText());
    m_cor_stack_size = 1024 * cor_stack_size; // 申请的是bit，需要的是byte 1byte=1024bit
    m_cor_pool_size = std::atoi(coroutine_node->FirstChildElement("coroutine_pool_size")->GetText());


    // msg_req_len
    TiXmlElement* msg_req_len_node = root->FirstChildElement("msg_req_len");
    if (!msg_req_len_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [msg_req_len] xml node\n", m_file_path.c_str());
        exit(0);
    }

    m_msg_req_len = std::atoi(root->FirstChildElement("msg_req_len")->GetText());

    // max_connect_timeout
    TiXmlElement* max_connect_timeout_node = root->FirstChildElement("max_connect_timeout");
    if (!max_connect_timeout_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [max_connect_timeout] xml node\n", m_file_path.c_str());
        exit(0);
    }

    int max_connect_timeout = std::atoi(root->FirstChildElement("max_connect_timeout")->GetText());
    m_max_connect_timeout = max_connect_timeout * 1000;

    // iothread_num
    TiXmlElement* iothread_num_node = root->FirstChildElement("iothread_num");
    if (!iothread_num_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [iothread_num] xml node\n", m_file_path.c_str());
        exit(0);
    }

    m_iothread_num = std::atoi(root->FirstChildElement("iothread_num")->GetText());

    // 时间轮配置：bucket_num和inteval
    TiXmlElement* time_wheel_node = root->FirstChildElement("time_wheel");
    if (!time_wheel_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [time_wheel] xml node\n", m_file_path.c_str());
        exit(0);
    }

    if (!time_wheel_node->FirstChildElement("bucket_num") || !time_wheel_node->FirstChildElement("bucket_num")->GetText())
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [time_wheel.bucket_num] xml node\n", m_file_path.c_str());
        exit(0);
    }
    if (!time_wheel_node->FirstChildElement("inteval") || !time_wheel_node->FirstChildElement("inteval")->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [time_wheel.bucket_num] xml node\n", m_file_path.c_str());
        exit(0);
    }

    m_timewheel_bucket_num = std::atoi(time_wheel_node->FirstChildElement("bucket_num")->GetText());
    m_timewheel_inteval = std::atoi(time_wheel_node->FirstChildElement("inteval")->GetText());


    // server ： 服务端使用ip,port, protocal
    TiXmlElement* server_node = root->FirstChildElement("server");
    if (!server_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [server] xml node\n", m_file_path.c_str());
        exit(0);
    }

    std::string ip = std::string(server_node->FirstChildElement("ip")->GetText());
    if (ip.empty()) 
    {
        ip = "0.0.0.0";
    }

    int port = std::atoi(server_node->FirstChildElement("port")->GetText());
    if (port == 0) 
    {
        printf("start tinyrpc server error! read config file [%s] error, read [server.port] = 0\n", m_file_path.c_str());
        exit(0);
    }
    std::string protocal = std::string(server_node->FirstChildElement("protocal")->GetText());
    std::transform(protocal.begin(), protocal.end(), protocal.begin(), toupper); // 给定范围，将字母变成大写，存回去

    tinyrpc::IPAddress::ptr addr = std::make_shared<tinyrpc::IPAddress>(ip ,port);
    if(protocal == "HTTP")
    {
        gRpcServer = std::make_shared<TcpServer>(addr, Http_Protocal);
    }
    else
    {
        gRpcServer = std::make_shared<TcpServer>(addr, TinyPb_Protocal);
    }

    char buff[512];
    sprintf(buff, "read config from file [%s]: [log_path: %s], [log_prefix: %s], [log_max_size: %d MB], [log_level: %s], " 
        "[coroutine_stack_size: %d KB], [coroutine_pool_size: %d], "
        "[msg_req_len: %d], [max_connect_timeout: %d s], "
        "[iothread_num:%d], [timewheel_bucket_num: %d], [timewheel_inteval: %d s], [server_ip: %s], [server_Port: %d], [server_protocal: %s]\n",
        m_file_path.c_str(), m_log_path.c_str(), m_log_prefix.c_str(), m_log_max_size / 1024 / 1024, 
        levelToString(m_log_level).c_str(), cor_stack_size, m_cor_pool_size, m_msg_req_len,
        max_connect_timeout, m_iothread_num, m_timewheel_bucket_num, m_timewheel_inteval, ip.c_str(), port, protocal.c_str()
    );
    
    std::string s(buff);
    InfoLog << s;


    // database: 数据库（mysql）配置
    TiXmlElement* database_node = root->FirstChildElement("database");
    if (!database_node) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [database] xml node\n", m_file_path.c_str());
        exit(0);
    }
    readDBConfig(database_node);
}
// 数据库（mysql）的配置
void Config::readDBConfig(TiXmlElement *node)
{
    // 还没使用数据库
}
// 日志的配置
void Config::readLogConfig(TiXmlElement *log_node)
{
    TiXmlElement* node = log_node->FirstChildElement("log_path");
    if(!node || !node->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [log_path] xml node\n", m_file_path.c_str());
        exit(0);
    }
    m_log_path = std::string(node->GetText());

    node = log_node->FirstChildElement("log_prefix");
    if(!node || !node->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [log_prefix] xml node\n", m_file_path.c_str());
        exit(0);
    }
    m_log_prefix = std::string(node->GetText());

    node = log_node->FirstChildElement("log_max_file_size");
    if(!node || !node->GetText()) {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [log_max_file_size] xml node\n", m_file_path.c_str());
        exit(0);
    }

    int log_max_size = std::atoi(node->GetText());
    m_log_max_size = log_max_size * 1024 * 1024; // kb -> MB 一个日志文件最多5MB


    node = log_node->FirstChildElement("rpc_log_level");
    if(!node || !node->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [rpc_log_level] xml node\n", m_file_path.c_str());
        exit(0);
    }

    std::string log_level = std::string(node->GetText());
    m_log_level = stringToLevel(log_level);

    node = log_node->FirstChildElement("app_log_level");
    if(!node || !node->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [app_log_level] xml node\n", m_file_path.c_str());
        exit(0);
    }

    log_level = std::string(node->GetText());
    m_app_log_level = stringToLevel(log_level);

    node = log_node->FirstChildElement("log_sync_inteval");
    if(!node || !node->GetText()) 
    {
        printf("start tinyrpc server error! read config file [%s] error, cannot read [log_sync_inteval] xml node\n", m_file_path.c_str());
        exit(0);
    }

    m_log_sync_inteval = std::atoi(node->GetText());

    gRpcLogger = std::make_shared<Logger>();
    gRpcLogger->init(m_log_prefix.c_str(), m_log_path.c_str(), m_log_max_size, m_log_sync_inteval);
}
TiXmlElement *Config::getXmlNode(const std::string &name)
{
    return m_xml_file->RootElement()->FirstChildElement(name.c_str());
}
}