#include "hook.h"
#include "fiber.h"
#include "iomanager.h"

#include <dlfcn.h>
#include <iostream>

#include "fd_manager.h"

namespace sylar { 

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() { 
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

struct _HookIniter {
    _HookIniter() {
        hook_init();
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

///下边定时器可能用到条件定时器，定义条件
struct timer_info {
    int cancelled = 0;
};


/**
 * @brief 实现一个统一的IO读写的函数
 * @param fd 文件描述符
 * @param fun 原始函数名
 * @param hook_fun_name Hook函数名
 * @param event 在IOManager中注册的IO事件
 * @param timeout_so 超时类型
 * @param args 参数
 */
template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, 
        uint32_t event, int timeout_so, Args&&... args) { 
    if(!sylar::t_hook_enable) {
        // 如果没有启用Hook，则直接调用系统函数 forward--展开函数的作用
        return fun(fd, std::forward<Args>(args)...);
    }
    
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        // 获取文件描述符实例 -- 失败 -- 不存在
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->m_isClosed()) {
        // 如果文件描述符已经关闭
        errno = EBADF;
        return -1;
    }

    if(!ctx->m_isSocket() || ctx->getUserNonblock()) {
        //如果不是socket或者用户设置了非阻塞
        return fun(fd, std::forward<Args>(args)...);
    }

    ///获取超时时间 -- 设置超时条件
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:

    ///执行函数方法--如果返回有效直接返回 -- n
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    /// 函数方法返回-1且错误码为EINTR--中断---那就重试
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }

    /// 重试后状态发生变化 -- 阻塞状态 -- 没有数据来 -- 进行IO操作
    if(n == -1 && errno == EAGAIN) {
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        sylar::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        /// 如果超时时间 ！= -1 
        if(to != (uint64_t)-1) {
            /// 添加条件定时器
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                // 当定时器回来后要将条件lock住，如果没有了说明失效了
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (sylar::FdContext::Event)(event));
            }, winfo);
        }

        /// 增加定时器
        int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
        if(rt) {
            /// 增加失败，取消定时器，报错退出
            SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            /// 增加成功，就YieldToHold，定时器回来后还存在的话，就取消定时器，
            sylar::Fiber::YieldToHold();
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {
                /// 说明是超时的，出错了，返回-1
                errno = tinfo->cancelled;
                return -1;
            }

            /// 当定时器回来后说明要执行任务，那就需要回到上面执行相关操作
            goto retry;
        }
    }

    return n;
}

extern "C" { 
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!sylar::t_hook_enable) {
        return sleep_f(seconds);
    }
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(seconds * 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    sylar::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!sylar::t_hook_enable) {
        return usleep_f(usec);
    }
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(usec / 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    sylar::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!sylar::t_hook_enable) {
        return nanosleep_f(req, rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(timeout_ms, [iom, fiber](){
        iom->schedule(fiber);
    });
    sylar::Fiber::YieldToHold();
    return 0;
}

}

