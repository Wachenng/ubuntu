#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

//定义线程局部变量指向【当前线程】
static thread_local Thread* t_thread = nullptr;
//定义线程局部变量指向【当前线程】的线程名称
static thread_local std::string t_thread_name = "UNKNOW";


static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    //当前线程以及当前线程所存储的线程局部变量的名字
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
                << " name=" << m_name;
        throw std::logic_error("pthread_create error");
    }
}

void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                    << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }        
    }
    m_thread = 0;
}


Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);        
    }
}

//这个函数是跑到真正的线程里边了
void* Thread::run(void* argc) {
    Thread* thread = (Thread*) argc;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();
    //给线程命名
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    cb();

    return 0;
}
    
} // namespace sylar