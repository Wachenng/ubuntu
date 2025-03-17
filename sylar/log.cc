#include "log.h"
#include <iostream>
#include <map>
#include <functional>
#include <stdarg.h>
#include <yaml-cpp/yaml.h>

namespace sylar {

/*********************************  LogLevel  *********************************/
const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level) {
#define XX(name) \
        case LogLevel::name: \
            return #name; \
            break;
    
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    return LogLevel::UNKNOW;
#undef XX
}

/*-------------------------    FormatItem的实例  -------------------------*/
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};


class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};


class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        //event中也有一个logger,用event的才是最原始的logger
        os << event->getLogger()->getName();
    }
};

    
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        
        os << buf;
    }
private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};
    
class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};





/*********************************  LogEvent  *********************************/
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse
    , uint32_t threadId, uint32_t fiberId, uint64_t time) 
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(threadId)
    ,m_fiberId(fiberId)
    ,m_time(time)
    ,m_logger(logger)
    ,m_level(level){
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}


LogEventWrap::LogEventWrap(LogEvent::ptr e) 
    :m_event(e) {
}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}



/*********************************  LogFormatter  *********************************/
LogFormatter::LogFormatter(const std::string& pattern) 
    :m_pattern(pattern) {
        init();
    }

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

//  %XXX    %XXX{XXX} 格式定义
void LogFormatter::init() {
    //string代表第一个XXX  string代表第二个XXX  int指对应类型    类型名-类型格式-包含不包含类型格式
    std::vector<std::tuple<std::string, std::string, int> > vec;
    //nstr是用来辨别不同item的即每一个%后面的第一个字符--后面要放上。
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i){
        if(m_pattern[i] !='%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i+1] =='%') {
                nstr.append(1, '%');
                continue;
            }            
        }


        //到这里%后面的第一个字符已经放进去nstr里了
        //处理下一个字符
        size_t n = i + 1;
        //即开始解析格式，因为我们一个字母处理后面跟的是formatitem的格式
        //0表示未解析或解析完成， 1表示开始解析以及解析中
        int fmt_status = 0;
        size_t fmt_begin = 0;

        //str是用来记录format名字所代表的格式的
        std::string str;
        //记录格式的str
        std::string fmt;
        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' 
                        && m_pattern[n] != '}' )) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0){
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()){
                //之前用nstr存了第一个字符，要放上
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            //再把解析出来的格式名称放上
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        } 
    }
    
    if(!nstr.empty()){
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    
    //定义Log4j的几种结构。
    // %m -- 消息体 
    // %p -- Level
    // %r -- 启动后时间 
    // %c -- 日志名称 
    // %t -- 线程id
    // %n -- 回车换行
    // %d -- 时间
    // %f -- 文件名
    // %l -- 行号 

    //函数模板 泛式函数
    //{"m", [](const std::string& fmt){ return FormatItem::ptr(new MessageFormatItem(fmt));}}
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr (new C(fmt));}}

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(F, FiberIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FileNameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        //std::cout << "{"  << std::get<0>(i) << " } - { " << std::get<1>(i) << " } - { " << std::get<2>(i) << "}" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}






/*********************************  LogAppender  *********************************/
void LogAppender::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
    if(m_formatter) {
        m_hasFormat = true;
    } else {
        m_hasFormat = false;
    }
}



/*----------   StdoutLogAppender  ----------*/
void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        std::cout <<  m_formatter->format(logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutAppender";
    if(m_level != LogLevel::UNKNOW) {
       node["level"] = LogLevel::ToString(m_level); 
    }
    if(m_hasFormat && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


/*----------   FileLogAppender  ----------*/
FileLogAppender::FileLogAppender(const std::string& filename) 
    :m_filename(filename) {
    reopen();
}

std::string FileLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW) {
       node["level"] = LogLevel::ToString(m_level); 
    }
    if(m_hasFormat && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}



/*********************************  Logger  *********************************/
Logger::Logger(const std::string& name)
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

void Logger::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;

    for(auto& i : m_appenders) {
        if(!i->m_hasFormat) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val) {
    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if(new_val->isError()){
        std::cout << " Logger set Formatter name= " << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl; 
        return;
    }
    //m_formatter = new_val;
    setFormatter(new_val);
}

LogFormatter::ptr Logger::getFormatter() {
    return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender){
    //增加Appender时如果Appender没有Formatter,就把logger本身的Formatter给他
    if(!appender->getFormatter()) {
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}


void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = m_appenders.begin();
            it != m_appenders.end(); ++it){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level){
        auto self = shared_from_this();
        //最开始通过Manager定义的logger的等级是最低的,且没有配置appender 所以处理一下
        if(!m_appenders.empty()) {                  //这个是给之后有appender的用的
            for(auto& i : m_appenders){
                i->log(self, level,event);
            } 
        } else if(m_root){                          //这个是给没有Appender的用的,暂时用他的root的log输出
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
}

std::string Logger::toYamlString() {
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW) {
       node["level"] = LogLevel::ToString(m_level); 
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();        
    }

    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();

}


/*----------  LoggerManager  ----------*/
LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->m_name] = m_root;

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    //没有就创建一个并把创建的这个的根root定义为manager最开始创建的root
    //创建完成后加到管理map中并返回
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString() {
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LoggerManager::init() {
    
}

}
