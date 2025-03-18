#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

namespace sylar {
    
/**********************  Semaphore  **********************/

class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();
private:
    //C++11 禁止默认拷贝
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete; 
    Semaphore& operator=(const Semaphore&) = delete; 

private:
    sem_t m_semaphore;
};

/**********************  互斥量  **********************/
//互斥量一般在局部的范围，所有加减锁一般通过类的构造函数加锁，析构函数解锁
template<class T>
struct ScopeLockImpl {
public:
    ScopeLockImpl(T& mutex)
        :m_mutex(mutex){
            m_mutex.lock();
            m_locked = true;
    }

    ~ScopeLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};


template<class T>
struct ReadScopeLockImpl {
public:
    ReadScopeLockImpl(T& mutex)
        :m_mutex(mutex){
            m_mutex.rdlock();
            m_locked = true;
    }

    ~ReadScopeLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};


template<class T>
struct WriteScopeLockImpl {
public:
    WriteScopeLockImpl(T& mutex)
        :m_mutex(mutex){
            m_mutex.wrlock();
            m_locked = true;
    }

    ~WriteScopeLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};


class RWMutex {
public:
    typedef ReadScopeLockImpl<RWMutex> ReadLock;
    typedef WriteScopeLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

} // namespace sylar




#endif