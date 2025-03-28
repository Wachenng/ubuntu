#include <assert.h>
#include "sylar/util.h"
#include "sylar/log.h"
#include "sylar/macro.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
void test_assert() {
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    SYLAR_ASSERT2(0 == 1, "test assert");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}