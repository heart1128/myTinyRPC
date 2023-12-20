// 注册tinypb服务，也就是将QueryService的实现类注册到dispatcher的map中

#ifndef SRC_COMM_START_H
#define SRC_COMM_START_H

#include <google/protobuf/service.h>
#include <memory>
#include <stdio.h>
#include <functional>
#include "src/comm/log.h"
#include "src/net/tcp/tcp_server.h"
#include "src/net/timer.h"


namespace tinyrpc {

// 注册到map中，在实际使用中调用注册
// #define REGISTER_SERVICE(service) \
//    do{ \
//         if(!tinyrpc::getServer()->registerService(std::make_shared<service>())) { \
//             printf("Start TinyRPC server error, because register protobuf service error, please look up rpc log get more details!\n"); \
//             tinyrpc::Exit(0); \  // 调用异步日志写入文件
//         } \
//     } while(0)\

// 使用do while(0)包含多个实现，防止逻辑错误
#define REGISTER_SERVICE(service) \
    do { \
    if (!tinyrpc::getServer()->registerService(std::make_shared<service>())) { \
        printf("Start TinyRPC server error, because register protobuf service error, please look up rpc log get more details!\n"); \
        tinyrpc::Exit(0); \
    } \
    } while(0)\



void initConfig(const char* file);

void startRpcServer();

TcpServer::ptr getServer();

int getIOThreadPoolSize();

Config::ptr getConfig();

void addTimerEvent(TimerEvent::ptr event);



} // namespace tinyrpc
#endif