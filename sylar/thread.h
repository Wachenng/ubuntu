#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

namespace sylar {

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id;}
    const std::string& getName() const { return m_name;}

    void join();

    //获取当前线程
    static Thread* GetThis();
    //获取当前线程名称
    static const std::string& GetName();
    //设置当前线程名称
    static void SetName(const std::string& name);
private:
    //C++11 禁止默认拷贝
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete; 
    Thread& operator=(const Thread&) = delete; 

    //线程真正的的执行方法
    static void* run(void* argc);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;
};

}

#endif