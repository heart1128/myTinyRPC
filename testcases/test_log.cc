#include <google/protobuf/service.h>
#include <iostream>
#include <pthread.h>
#include "src/coroutine/coroutine_pool.h"
#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/coroutine/coroutine.h"
#include "src/net/mutex.h"

int main(int argc, char* argv[])
{
    DebugLog << "test log.cc";
    return 0;
}