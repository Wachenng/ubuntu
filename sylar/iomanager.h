#ifndef __SYLAR_IO_MANAGER_H__
#define __SYLAR_IO_MANAGER_H__

#include "scheduler.h"


/*
  IOManager(epoll) --> Scheduler
    |
    |
    v
  idle(epoll_wait)

     信号量
  putMessage(msg,) + 信号量1
  message_queue
        |
        |--------Thread
        |--------Thread
            wait()-信号量1 RecvMessage(msg,1)
    PS：如果信号量没有，那么就会一直阻塞在这里，存在问题是如果信号量阻塞，那么整个线程都被阻塞了


异步IO, 等待数据返回。 epoll_wait()如果没有数据会阻塞在epoll_wait()上，直到有数据返回。或有人向里边放任务
*/
namespace sylar {

class IOManager : public Scheduler{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE    = 0x0,
        READ    = 0x1,
        WRITE   = 0x4
    };

private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr;         //表示在哪一个调度器上执行
            Fiber::ptr fiber;                       //表示要执行的fiber
            std::function<void()> cb;               //表示要执行的函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        int fd = 0;                  //事件描述符
        EventContext read;       //读事件
        EventContext write;      //写事件
        Event events = NONE;   //已经注册的事件
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    /**
     * @brief 增加事件
     * @param
     * @return 0 success -1 error
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件--直接删除了
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件--如果存在触发条件，那么先触发事件再删除事件
     */
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    /**
     * @brief 获取当前IOManager
     */
    static IOManager * GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void contextResize(size_t size);
private:
    int m_epfd = 0;
    //通过管道来唤醒，不通过异步IO来唤醒，即进程间通讯方式
    //不通过信号量+1-1的方式，而是通过向里边写数据的方式
    int m_tickleFds[2];
    //现在等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

}

#endif