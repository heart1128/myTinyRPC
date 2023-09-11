#ifndef SRC_COROUTINE_COUROUTINE_HOOK_H
#define SRC_COROUTINE_COUROUTINE_HOOK_H

#include <unistd.h> // read, write in linux
#include <sys/socket.h>
#include <sys/types.h>

#include <functional>

// hook系统函数的函数指针

typedef ssize_t (*read_fun_ptr_t)(int fd, void *buf, size_t count);

typedef ssize_t (*write_fun_ptr_t)(int fd, const void *buf, size_t count);

typedef int (*connect_fun_ptr_t)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

typedef int (*accept_fun_ptr_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

typedef int (*socket_fun_ptr_t)(int domain, int type, int protocol);

typedef int (*sleep_fun_ptr_t)(unsigned int seconds);
// 这里不适合使用std::function， 因为后面要使用函数指针作为返回值，std::function不支持void*到定义类型的转换，普通的函数指针可以
// std::function<ssize_t(int, void*, size_t)> read_fun_ptr_t; // 系统read hook

// std::function<ssize_t(int, const void*, size_t)> write_fun_ptr_t; // 系统write hook

// std::function<int(int, const struct sockaddr*, socklen_t)> connect_fun_ptr_t; // connect hook

// std::function<int(int, struct sockaddr*, socklen_t)> accept_fun_ptr_t;  // accept hook

// std::function<int(int, int, int)> socket_fun_ptr_t; // socket hook

// std::function<int(unsigned int)> sleep_fun_ptr_t;  // sleep hook


namespace tinyrpc{

int accept_hook(int sockf, struct sockaddr* addr, socklen_t* addrlen);

ssize_t read_hook(int fd, void* buf, size_t count);

ssize_t write_hook(int fd, const void* buf, size_t count);

int connect_hook(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

unsigned int sleep_hook(unsigned int seconds);

void setHook(bool);


extern "C"{
    int accept(int sockf, struct sockaddr* addr, socklen_t* addrlen);

    ssize_t read(int fd, void* buf, size_t count);

    ssize_t write(int fd, const void* buf, size_t count);

    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

    unsigned int sleep(unsigned int seconds);
}


} // namespace tinyrp



#endif