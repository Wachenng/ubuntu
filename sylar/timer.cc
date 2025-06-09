#include "timer.h"
#include "util.h"

namespace sylar {

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if(!lhs && !rhs)
        return false;
    if(!lhs)
        return true;
    if(!rhs)
        return false;
    if(lhs->m_next < rhs->m_next)
        return true;
    if(rhs->m_next < lhs->m_next)
        return false;
    return lhs.get() < rhs.get();
}


Timer::Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager)
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) {
    m_next = sylar::GetCurrentMS() + m_ms;
}


TimerManager::TimerManager() {

}

TimerManager::~TimerManager() {

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutex::WriteLock lock(m_mutex);
    auto it = m_timers.insert(timer).first;
    //判断是否插入到最前面的位置--如果是说明插入的位置是最小的时间-->就要通知进程一个新的最小定时器放到前面了
    //之前epoll_wait的那个定时器可能太大了，需要回来重新设置一下时间
    bool at_front = (it == m_timers.begin());
    lock.unlock();

    if(at_front) {
        onTimerInsertedAtFront();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
               ,std::weak_ptr<void> weak_cond, bool recurring) {

}

}