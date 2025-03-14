#include "../sylar/config.h"
#include "../sylar/log.h"

sylar::ConfigVar<int>::ptr g_int_val_config = sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_int_float_val_config = sylar::Config::Lookup("system.value", (float)10.2f, "system value");

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_val_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_val_config->toString();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_float_val_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_float_val_config->toString();
    return 0;
}