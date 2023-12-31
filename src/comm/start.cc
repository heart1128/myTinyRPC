#include <google/protobuf/service.h>
#include "src/comm/start.h"
#include "src/comm/log.h"
#include "src/comm/config.h"
#include "src/net/tcp/tcp_server.h"
#include "src/coroutine/coroutine_hook.h"

namespace tinyrpc {

tinyrpc::Config::ptr gRpcConfig;
tinyrpc::Logger::ptr gRpcLogger;
tinyrpc::TcpServer::ptr gRpcServer;

static int g_init_config = 0;
    
void initConfig(const char *file)
{
    tinyrpc::setHook(true);
    if (g_init_config == 0) 
    {
        gRpcConfig = std::make_shared<tinyrpc::Config>(file);
        gRpcConfig->readConf();
        g_init_config = 1;
    }
}

TcpServer::ptr getServer() 
{
    return gRpcServer;
}

void startRpcServer() 
{
    gRpcLogger->start();
    gRpcServer->start();
}

int getIOThreadPoolSize() 
{
    return gRpcServer->getIOThreadPool()->getIOThreadPoolSize();
}

Config::ptr getConfig() 
{
    return gRpcConfig;
}

void addTimerEvent(TimerEvent::ptr event) 
{

}
    
} // namespace tinyrpc


