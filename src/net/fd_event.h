#ifndef SRC_NET_FD_EVNET_H
#define SRC_NET_FD_EVNET_H

#include <functional>
#include <memory>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <assert.h>
#include "reactor.h"
#include "src/comm/log.h"
#include "src/coroutine/coroutine.h"
#include "mutex.h"

namespace tinyrpc{

class Reactor;

enum IOEvent{
    READ = EPOLLIN, // 0x001
    WRITE = EPOLLOUT, //0x004
    ETModel = EPOLLET //1u << 31
};

/**
 * 
 * 
 * ---------------- fdEnvnt   fd时间类
 * 
 * 继承 enable_shared_from_this<T> 目的是为了继承hared_from_this函数，该函数返回一个由智能指针管理的this, 如果要将this返回给其他函数使用
 * 使用这个方法能够防止当这个实例析构之后其他函数还不知道，继续使用。使用智能指针就能避免。
 * 替代 std::shared_ptr(this) 返回this,这样如果会引起两次析构，因为有两次引用计数，this是裸指针，使用裸指针会构造一个新的引用计数
 * 使用shared_from_this是返回一个和本次实例共享引用计数的智能指针，实例离开作用域之后就不会进行析构，因为还有引用计数
 * 
 * 继承enable_shared_from_this，内部构造一个FdEvent类，子类又是使用这个继承类，所以返回的this智能指针就有两个引用计数，使用时就不会因为析构出现问题了
 * 
 * 在要返回this指针使用最好
 * https://blog.csdn.net/breadheart/article/details/112451022
*/

class FdEvent : public std::enable_shared_from_this<FdEvent>{
public:
    typedef std::shared_ptr<FdEvent> ptr;

public:
    FdEvent(tinyrpc::Reactor* reactor, int fd = -1);
    FdEvent(int fd);

    virtual ~FdEvent();  // 因为是继承，所以使用虚析构防止未被析构

public:
    // 处理read or write事件,执行保存的回调函数
    void handleEvent(IOEvent flag);
    // 设置回调函数，设置类型
    void setCallBack(IOEvent flag, std::function<void()> cb);
    std::function<void()> getCallBack(IOEvent flag) const;
    // 添加监听时间  read or write
    void addListenEvents(IOEvent event);
    void delListenEvents(IOEvent event);
    // 设置fd和事件到reactor的epoll中
    void updateToReactor();
    // 取消本fd之前注册的所有read和write事件
    void unregisterFromReactor ();

    int getFd() const;

    void setFd(const int fd);

    int getListenEvents() const;

    Reactor* getReactor() const;

    void setReactor(Reactor* r);

    void setNonBlock();
    
    bool isNonBlock();

    void setCoroutine(Coroutine* cor);

    Coroutine* getCoroutine();

    void clearCoroutine();

public:
    Mutex m_mutex;

protected:
    int m_fd {-1};      // fdevent中的fd
    std::function<void()> m_read_callback;  // read callback
    std::function<void()> m_write_callback;

    int m_listen_events {0};  // 所有的监听事件

    Reactor* m_reactor {nullptr};  

    Coroutine* m_coroutine {nullptr};
};

/**
 * 
 * 
 * ---------------- fd事件容器 
 * 
*/

class FdEventContainer{

public:
    FdEventContainer(int size);     // 初始化容器大小
    FdEvent::ptr getFdEvent(int fd);  // 指定fd从容器中拿出FdEvent

public:
    static FdEventContainer* getFdContainer();  // 单例模式，仅仅使用静态函数，不使用对象，使用这个对象调用静态函数 ，在内存中只有一个静态实例 

private:
    RWMutex m_mutex;
    std::vector<FdEvent::ptr> m_fds;

};


    
} // namespace tinyrpc



#endif