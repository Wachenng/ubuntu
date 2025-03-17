#include "log.h"
#include "config.h"

/**
 * @brief 日志系统类型定义
 * logs:
 *    - name: root
 *      level: (debug,info,warn,error,fatal)
 *      formatter: "%d%T%p%T%t%m%n"
 *      appender:
 *          - type:  (StdoutLogAppender, FileLogAppender)
 *            level: (debug...)
 *            file: /logs/xxx.log
 */

namespace sylar {

struct LogAppenderDefine {
    int type = 0;   //1 File 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

//定义log_config配置文件
sylar::ConfigVar<std::set<LogDefine> > g_log_definses =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

//定义注册事件
struct LogIniter {
    LogIniter() {
        g_log_definses->addListener(0xF1E231, [](const std::set<LogDefine> & old_value,
                    const std::set<LogDefine> & new_value){

        });
    }
};

static LogIniter __log_init;

}