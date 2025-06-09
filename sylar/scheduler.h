#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

/*
       1-----N     1-------M
scheduler --> thread  ---> filer

1.线程池，分配一组线程
2.协程调度器，将协程，指定到相应的线程上去执行

N : M
*/

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"
#include "mutex.h"
#include "log.h"

namespace sylar {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 协程调度器构造函数
     * @param thread_num 线程数量
     * @param use_caller main函数所处的线程是否纳入到协程调度器当中管理
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    //获取当前协程调度器
    static Scheduler * GetThis();

    //获取调度器的主协程-->与线程的主协程是不一样的
    static Fiber* GetMainFiber();

    //启动
    void start();

    //停止
    void stop();

    //单个线程放入
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle) {
            tickle();
        }
    }

    //批量线程放入
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }

protected:
    virtual void tickle();
    void run();
    virtual bool stopping();
    virtual void idle();

    void setThis();

    bool hasIdleThreads() { return m_idleThreadCount > 0;}
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    //调度器可以支持的协程：包括（函数，协程）
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        //指定协程是在那个线程上运行
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f)
            ,thread(thr){}

        //指定协程是在那个线程上运行
        //传入智能指针的形式用于管理内存，智能指针的智能指针的引用计数会减一
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr){
            fiber.swap(*f);
        }

        //指定函数在那个线程上运行
        FiberAndThread(std::function<void()> f, int thread)
            :cb(f)
            ,thread(thread){}

        //指定函数在那个线程上运行
        FiberAndThread(std::function<void()>* f, int thread)
            :thread(thread){
            cb.swap(*f);
        }

        //默认构造函数
        FiberAndThread()
            :thread(-1){}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }        
    };

private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;             //主协程
    std::string m_name;

protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount {0};
    std::atomic<size_t> m_idleThreadCount {0};
    bool m_stopping = true;
    bool m_autostop = false;
    int m_rootThread = 0;
};



}



#endif