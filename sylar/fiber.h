#ifndef __SYLAR__FIBER_H__
#define __SYLAR__FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"


namespace sylar{

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
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

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = State::INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};


} // namespace sylar

#endif