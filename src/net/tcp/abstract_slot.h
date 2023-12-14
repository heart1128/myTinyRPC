#ifndef SRC_NET_TCP_ABSTRACTSLOT_H
#define SRC_NET_TCP_ABSTRACTSLOT_H

#include <memory>
#include <functional>

namespace tinyrpc{

template<class T>
class AbstractSlot{
public:
    using ptr = std::shared_ptr<AbstractSlot>;

    using weakPtr = std::weak_ptr<T>;      // tcp连接共享指针
    using sharedPtr = std::shared_ptr<T>;  // tcp连接智能指针

    AbstractSlot(weakPtr ptr, std::function<void(sharedPtr)> cb)
    : m_weak_ptr(ptr), m_cb(cb)
    {}

    ~AbstractSlot(){
        sharedPtr ptr = m_weak_ptr.lock(); // weak_ptr 升级成shared_ptr，影响引用计数
        if(ptr)
        {
            m_cb(ptr); // 每次只是使用一个引用计数，然后使用weak_ptr监视，最后析构的时候升级进行引用计数清零。保证一直只有一个引用计数。
        }
    }

private:
    weakPtr m_weak_ptr;
    std::function<void(sharedPtr)> m_cb;

};
    
} // namespace tinyrpc


#endif