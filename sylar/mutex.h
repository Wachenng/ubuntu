#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <atomic>

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

class Mutex {
public:
    typedef ScopeLockImpl<Mutex> Lock;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    //C++11 禁止默认拷贝
    Mutex(const Mutex&) = delete;
    Mutex(const Mutex&&) = delete; 
    Mutex& operator=(const Mutex&) = delete; 
private:
    pthread_mutex_t m_mutex;
};


class NullMutex {
public:
    typedef ScopeLockImpl<NullMutex> Lock;

    NullMutex() {}

    ~NullMutex() {}

    void lock() {}

    void unlock() {}
private:
    //C++11 禁止默认拷贝
    NullMutex(const NullMutex&) = delete;
    NullMutex(const NullMutex&&) = delete; 
    NullMutex& operator=(const NullMutex&) = delete; 
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
    //C++11 禁止默认拷贝
    RWMutex(const RWMutex&) = delete;
    RWMutex(const RWMutex&&) = delete; 
    RWMutex& operator=(const RWMutex&) = delete; 

private:
    pthread_rwlock_t m_lock;
};


class NullRWMutex {
public:
    typedef ReadScopeLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopeLockImpl<NullRWMutex> WriteLock;

    NullRWMutex() {}

    ~NullRWMutex() {}

    void rdlock() {}

    void wrlock() {}

    void unlock() {}
private:
    //C++11 禁止默认拷贝
    NullRWMutex(const NullRWMutex&) = delete;
    NullRWMutex(const NullRWMutex&&) = delete; 
    NullRWMutex& operator=(const NullRWMutex&) = delete; 
};


class Spinlock {
public:
    typedef ScopeLockImpl<Spinlock> Lock;
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    //C++11 禁止默认拷贝
    Spinlock(const Spinlock&) = delete;
    Spinlock(const Spinlock&&) = delete; 
    Spinlock& operator=(const Spinlock&) = delete; 
private:
    pthread_spinlock_t m_mutex;
};


class CASLock {
public:
    typedef ScopeLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {

    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }   
private:
    //C++11 禁止默认拷贝
    CASLock(const CASLock&) = delete;
    CASLock(const CASLock&&) = delete; 
    CASLock& operator=(const CASLock&) = delete; 
private:
    volatile std::atomic_flag m_mutex;
};



} // namespace sylar




#endif