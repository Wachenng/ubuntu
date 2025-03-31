#ifndef __SYLAR__FIBER_H__
#define __SYLAR__FIBER_H__

/*
Fiber::GetThis() --> 创建主协程
Thread->main_fiber <--------------> sub_fiber
            ^
            |
            v
         sub_fiber
*/


#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"


namespace sylar{

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,           //// 初始状态
        HOLD,           //// 暂停状态
        EXEC,           //// 执行状态
        TERM,           //// 结束状态
        READY,          //// 就绪状态
        EXCEPT          //// 异常状态
    };

private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();

    // 重置fiber状态
    void reset(std::function<void()> cb);
    // 切换到当前协程执行
    void swapIn();
    // 切换到后台执行
    void swapOut();
    uint64_t getId() const { return m_id;}

public:
    //设置当前协程
    static void SetThis(Fiber* f);
    // 获取当前协程
    static Fiber::ptr GetThis();
    // 协程切换到后台, 并且设置为Ready状态
    static void YieldToReady();
    // 协程切换到后台, 并且设置为Hold状态
    static void YieldToHold();
    // 获取协程数量
    static uint64_t TotalFibers();

    static void MainFunc();
    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};


} // namespace sylar

#endif