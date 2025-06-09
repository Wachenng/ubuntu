#include "util.h"
#include <execinfo.h>
#include <sys/time.h>

#include "log.h"
#include "fiber.h"

namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return sylar::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bts, int size, int skip) {
    bts.reserve(size);
    void** buffer = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(buffer, size);

    char** strings = backtrace_symbols(buffer, s);
    if(strings == nullptr) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "backtrace_symbols error";
        return;
    }
    for(size_t i = skip; i < s; ++i) {
        bts.push_back(strings[i]);
    }
    free(strings);
    free(buffer);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bts;
    Backtrace(bts, size, skip);
    std::stringstream ss;
    for(size_t i = skip; i < bts.size(); ++i) {
        ss << prefix << bts[i] << std::endl;
    }
    return ss.str();
}

uint64_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

}