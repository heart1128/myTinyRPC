cmake_minimum_required(VERSION 3.0) # cmake最小版本要求
project(myTinyRpc)  # 设定工程名

SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}) # 设置install时的前缀为项目目录名，也就是项目根目录mytinyrpc

enable_language(ASM) # 支持汇编，协程汇编      

# 设置基本编译命令
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable")

set(PATH_LIB lib)  # 存放库文件目录
set(PATH_BIN bin)  # 
set(PATH_TESTCASES testcases)

include_directories(${PROJECT_SOURCE_DIR}) # 指的是和CmakeList.txt平行的目录,也就是项目根目录
## *.cc
aux_source_directory(${PROJECT_SOURCE_DIR}/src/comm COMM)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/coroutine COR)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/tcp TCP)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/tinypb TINYPB)
# 汇编
set(COXTX ${PROJECT_SOURCE_DIR}/src/coroutine/coctx_swap.S)


# 添加库，就是cc文件 
add_library(myTinyRpc ${COMM} ${COR} ${NET} ${TCP} ${TINYPB} ${COXTX})
install(TARGETS myTinyRpc DESTINATION ${PATH_LIB})

############ 设置protobuf
find_package(Protobuf REQUIRED) # 寻找第三方需要的包，REQUIRED是自带参数
if(${PROTOBUF_FOUND})  # https://www.jianshu.com/p/2946b0e5c45b cmake自带对protobuf的查找，使用PROTOBUF_FOUND判断
    message("protobuf found") # 打印echo
else ()
    message(FATAL_ERROR "Cannot find Protobf")
endif()

############ 设置tinyxml
set(tinyxml /usr/lib/libtinyxml.a) # cmake没有自带的搜索tinyxml，自己指定库位置


############ 设置所有libs变量
set(LIBS
    myTinyRpc
    ${Protobuf_LIBPARY}
    ${tinyxml}
    dl          # hook用
    pthread
)

# test_tinypb_server  使用了proto编译的.cc文件也要加进去
set(
    test_tinypb_server
    ${PROJECT_SOURCE_DIR}/${PATH_TESTCASES}/test_tinypb_server.cc
    ${PROJECT_SOURCE_DIR}/${PATH_TESTCASES}/test_tinypb_server.pb.cc
)

# test_tinypb_server_client
set(
    test_tinypb_server_client
    ${PROJECT_SOURCE_DIR}/${PATH_TESTCASES}/test_tinypb_server_client.cc
    ${PROJECT_SOURCE_DIR}/${PATH_TESTCASES}/test_tinypb_server.pb.cc
)

# test_coroutine
set(
    test_coroutine
    ${PROJECT_SOURCE_DIR}/${PATH_TESTCASES}/test_coroutine.cc
)

add_executable(test_coroutine ${test_coroutine})
target_link_libraries(test_coroutine ${LIBS}) # 显式指定静态链接库
install(TARGETS test_coroutine DESTINATION ${PATH_BIN}) # 可执行文件


# install *.h
add_subdirectory(src/comm)
add_subdirectory(src/coroutine)
add_subdirectory(src/net)
add_subdirectory(src/net/tcp)
add_subdirectory(src/net/tinypb)
