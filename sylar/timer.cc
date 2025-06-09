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

Timer::Timer(uint64_t next)
    :m_next(next) {
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = sylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = sylar::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager() {
    m_previouse_time = sylar::GetCurrentMS();
}

TimerManager::~TimerManager() {

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutex::WriteLock lock(m_mutex);
    addTimer(timer, lock);

    // 返回该定时器是为了刚设定立马取消的时候可以直接取消，不用再重新查找然后取消了
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    // lock()返回智能指针，如果weak_ptr没有被释放，则返回一个指向该对象的智能指针，否则返回null
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
               ,std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutex::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) {
        return ~0ull;
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = sylar::GetCurrentMS();

    // 因为某种原因晚了，就立即执行
    if(now_ms >= next->m_next) {
        return 0;
    } else {//没有就返回下一个时间
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutex::ReadLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
    }

    RWMutex::WriteLock lock(m_mutex);

    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)){
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    //二分查找当前时间
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first;
    //判断是否插入到最前面的位置--如果是说明插入的位置是最小的时间-->就要通知进程一个新的最小定时器放到前面了
    //之前epoll_wait的那个定时器可能太大了，需要回来重新设置一下时间
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();

    if(at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;

    if(now_ms < m_previouse_time && 
        now_ms < (m_previouse_time - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouse_time = now_ms;
    return rollover;
}

bool TimerManager::hasTimer() {
    RWMutex::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}