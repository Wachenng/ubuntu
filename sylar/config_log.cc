#include "log.h"
#include "config.h"
#include <iostream>
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

//自定义类的片特化
template<>
class LexicalCast<std::string, std::set<LogDefine> > {
public:
    std::set<LogDefine> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;
        for(size_t i = 0; i < node.size(); ++i) {
            auto n = node[i];
            if(!n["name"].IsDefined()) {
                std::cout << " log config error: name is null, " << n 
                          << std::endl;
                continue;
            }

            LogDefine ld;
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");

            if(n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if(n["appenders"].IsDefined()) {
                for(size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];

                    if(!a["type"].IsDefined()) {
                        std::cout << " log config error: appender type is null, " << a
                                << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if(type == "FileLogAppender") {
                        lad.type = 1;
                        if(!a["file"].IsDefined()) {
                            std::cout << " log config error: fileappender file is null, " << a
                                    << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if(a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if(type == "StdoutLogAppender") {
                        lad.type = 2;
                    } else {
                        std::cout << " log config error: appender type is invalid, " << a
                                << std::endl;
                        continue;
                    }
                    
                    ld.appenders.push_back(lad);
                }
            }
            vec.insert(ld);
        }
        return vec;
    }
};

template<>
class LexicalCast<std::set<LogDefine>, std::string> {
public:
    std::string operator()(const std::set<LogDefine>& v) {
        YAML::Node node;
        for(auto& i : v) {
            YAML::Node n;
            n["name"] = i.name;
            if(i.level != LogLevel::UNKNOW){
                n["level"] = LogLevel::ToString(i.level);                
            }
            if(!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for(auto& a : i.appenders) {
                YAML::Node na;
                if(a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if(a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKNOW){
                    na["level"] = LogLevel::ToString(a.level);
                }

                if(!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//定义log_config配置文件
sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_definses =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

//定义注册事件
struct LogIniter {
    LogIniter() {
        g_log_definses->addListener(0xF1E231, [](const std::set<LogDefine> & old_value,
                    const std::set<LogDefine> & new_value){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << " on_logger_conf_changed";
            //新增
            for(auto& i : new_value){
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增加logger
                    logger = SYLAR_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                }

                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    sylar::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new sylar::FileLogAppender(a.file));
                    } else if (a.type == 2) {
                        ap.reset(new StdoutLogAppender);
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()){
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()){
                            ap->setFormatter(fmt);
                        }else {
                            std::cout << "log.name = " << i.name << " appender type="  << a.type << " formatter" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;

}