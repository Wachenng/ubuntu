#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__ 

#include <memory>
#include <vector>
#include <set>
#include "fiber.h"
// Timer --> addTimer() --->cancel()
// 获取当前的定时器触发离现在的时间差
// 返回当前需要触发的定时器

namespace sylar { 

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> { 
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    // 取消定时器
    bool cancel();

    //刷新设定定时器的执行时间
    bool refresh();

    // 是否从当时时间为基准重置时间
    bool reset(uint64_t ms, bool from_now);

private:
    /**
     * @brief 通过TimerManager创建定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] recurring 是否循环执行
     * @param[in] manager 定时器管理器
     */
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);

    Timer(uint64_t next);

private:
    // 是否循环
    bool m_recurring = false;
    // 定时周期
    uint64_t m_ms = 0;
    // 精确的执行时间 11：55：28
    uint64_t m_next = 0;
    std::function<void()> m_cb;

    TimerManager* m_manager = nullptr;

private:
    // 因为Manager中有定时器列表，需要判断是否超时，要进行比较，所以增加比较函数
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};


class TimerManager {
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    /**
     * @brief 添加一个定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] recurring 是否循环执行
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief 添加条件定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] weak_cond 条件定时器依赖条件
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
               ,std::weak_ptr<void> weak_cond, bool recurring = false);

    // 获取下一个定时器的执行时间
    uint64_t getNextTimer();

    // 返回超时以及需要执行的Timer的回调函数
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    // 是否有定时器
    bool hasTimer();
protected:
    virtual void onTimerInsertedAtFront() = 0;
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

private:
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    bool m_tickled = false;
    uint64_t m_previouse_time = 0;
};

}

#endif