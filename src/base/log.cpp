/**
 * @file log.cpp
 * @brief 日志模块
 * @date 2026-03-27
 */

#include "base/log.h"

namespace zch {

const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name) case LogLevel::name: return #name;
    XX(FATAL);
    XX(ALERT);
    XX(CRIT);
    XX(ERROR);
    XX(WARN);
    XX(NOTICE);
    XX(INFO);
    XX(DEBUG);
#undef XX
    default:
        return "NOTSET";
    }
    return "NOTSET";
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) if(str == #v) { return LogLevel::level; }
    XX(FATAL, fatal);
    XX(ALERT, alert);
    XX(CRIT, crit);
    XX(ERROR, error);
    XX(WARN, warn);
    XX(NOTICE, notice);
    XX(INFO, info);
    XX(DEBUG, debug);

    XX(FATAL, FATAL);
    XX(ALERT, ALERT);
    XX(CRIT, CRIT);
    XX(ERROR, ERROR);
    XX(WARN, WARN);
    XX(NOTICE, NOTICE);
    XX(INFO, INFO);
    XX(DEBUG, DEBUG);
#undef XX

    return LogLevel::NOTSET;
}

LogEvent::LogEvent(const std::string &logger_name, LogLevel::Level level, 
    const char *file, int32_t line, int64_t elapse, time_t time)
    : m_level(level)
    , m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_time(time)
    , m_loggerName(logger_name) {
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << event->GetContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << LogLevel::ToString(event->GetLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << event->GetElapse();
    }
};

class LoggerNameFormatItem : public LogFormatter::FormatItem {
public:
    LoggerNameFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << event->GetLoggerName();
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

    void Format(std::ostream& os, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->GetTime();
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
    FileNameFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << event->GetFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << event->GetLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str) : m_string(str) {}
    void Format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << "\t";
    }
};

class PercentSignFormatItem : public LogFormatter::FormatItem {
public:
    PercentSignFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::ptr event) override {
        os << "%";
    }
};

LogFormatter::LogFormatter(const std::string &pattern)
    : m_pattern(pattern) {
    Init();
}

/**
 * 简单的状态机判断，提取pattern中的常规字符和模式字符
 * 
 * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
 * 
 * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
 * 
 * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这个状态需要特殊处理
 * 
 * 一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
 * 
 * @see LogFormatter::LogFormatter
 */
void LogFormatter::Init() {
    // 按顺序存储解析到的pattern项
    // 每个pattern包括一个整数类型和一个字符串，类型为0表示该pattern是常规字符串，为1表示该pattern需要转义
    // 日期格式单独用下面的dataformat存储
    std::vector<std::pair<int, std::string>> patterns;
    // 临时存储常规字符串
    std::string tmp;
    // 日期格式字符串，默认把位于%d后面的大括号对里的全部字符都当作格式字符，不校验格式是否合法
    std::string dateformat;
    // 是否解析出错
    bool error = false;
    
    // 是否正在解析常规字符，初始时为true
    bool parsing_string = true;
    // 是否正在解析模板字符，%后面的是模板字符
    // bool parsing_pattern = false;

    size_t i = 0;
    while(i < m_pattern.size()) {
        std::string c = std::string(1, m_pattern[i]);
        if(c == "%") {
            if(parsing_string) {
                if(!tmp.empty()) {
                    patterns.push_back(std::make_pair(0, tmp));
                }
                tmp.clear();
                parsing_string = false; // 在解析常规字符时遇到%，表示开始解析模板字符
                // parsing_pattern = true;
                i++;
                continue;
            } else {
                patterns.push_back(std::make_pair(1, c));
                parsing_string = true; // 在解析模板字符时遇到%，表示这里是一个%转义
                // parsing_pattern = false;
                i++;
                continue;
            }
        } else { // not %
            if(parsing_string) { // 持续解析常规字符直到遇到%，解析出的字符串作为一个常规字符串加入patterns
                tmp += c;
                i++;
                continue;
            } else { // 模板字符，直接添加到patterns中，添加完成后，状态变为解析常规字符，%d特殊处理
                patterns.push_back(std::make_pair(1, c));
                parsing_string = true; 
                // parsing_pattern = false;

                // 后面是对%d的特殊处理，如果%d后面直接跟了一对大括号，那么把大括号里面的内容提取出来作为dateformat
                if(c != "d") {
                    i++;
                    continue;
                }
                i++;
                if(i < m_pattern.size() && m_pattern[i] != '{') {
                    continue;
                }
                i++;
                while( i < m_pattern.size() && m_pattern[i] != '}') {
                    dateformat.push_back(m_pattern[i]);
                    i++;
                }
                if(m_pattern[i] != '}') {
                    // %d后面的大括号没有闭合，直接报错
                    std::cout << "[ERROR] LogFormatter::Init() " << "pattern: [" << m_pattern << "] '{' not closed" << std::endl;
                    error = true;
                    break;
                }
                i++;
                continue;
            }
        }
    } // end while(i < m_pattern.size())

    if(error) {
        m_error = true;
        return;
    }

    // 模板解析结束之后剩余的常规字符也要算进去
    if(!tmp.empty()) {
        patterns.push_back(std::make_pair(0, tmp));
        tmp.clear();
    }
    
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C)  {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));} }

        XX(m, MessageFormatItem),           // m:消息
        XX(p, LevelFormatItem),             // p:日志级别
        XX(c, LoggerNameFormatItem),        // c:日志器名称
        XX(d, DateTimeFormatItem),          // d:日期时间
        XX(r, ElapseFormatItem),            // r:累计毫秒数
        XX(f, FileNameFormatItem),          // f:文件名
        XX(l, LineFormatItem),              // l:行号
        XX(%, PercentSignFormatItem),       // %:百分号
        XX(T, TabFormatItem),               // T:制表符
        XX(n, NewLineFormatItem),           // n:换行符
#undef XX
    };

    for(auto &v : patterns) {
        if(v.first == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
        } else if( v.second =="d") {
            m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
        } else {
            auto it = s_format_items.find(v.second);
            if(it == s_format_items.end()) {
                std::cout << "[ERROR] LogFormatter::Init() " << "pattern: [" << m_pattern << "] " << 
                "unknown format item: " << v.second << std::endl;
                error = true;
                break;
            } else {
                m_items.push_back(it->second(v.second));
            }
        }
    }

    if(error) {
        m_error = true;
        return;
    }
}

std::string LogFormatter::Format(LogEvent::ptr event) {
    std::stringstream ss;
    for(auto &i:m_items) {
        i->Format(ss, event);
    }
    return ss.str();
}

std::ostream &LogFormatter::Format(std::ostream &os, LogEvent::ptr event) {
    for(auto &i:m_items) {
        i->Format(os, event);
    }
    return os;
}

LogAppender::LogAppender(LogFormatter::ptr default_formatter)
    : m_defaultFormatter(default_formatter) {
}

void LogAppender::SetFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
}

LogFormatter::ptr LogAppender::GetFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter ? m_formatter : m_defaultFormatter;
}

StdoutLogAppender::StdoutLogAppender()
    : LogAppender(LogFormatter::ptr(new LogFormatter)) {
}

void StdoutLogAppender::Log(LogEvent::ptr event) {
    MutexType::Lock lock(m_mutex);
    if(m_formatter) {
        m_formatter->Format(std::cout, event);
    } else {
        m_defaultFormatter->Format(std::cout, event);
    }
}

std::string StdoutLogAppender::ToYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["pattern"] = m_formatter->GetPattern();
    std::stringstream ss;
    ss << node;

    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string &file)
    : LogAppender(LogFormatter::ptr(new LogFormatter)) {
    m_filename = file;
    ReOpen();
    if(m_reopenError) {
        std::cout << "reopen file " << m_filename << " error" << std::endl;
    }
}

/**
 * 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
 */
void FileLogAppender::Log(LogEvent::ptr event) {
    uint64_t now = event->GetTime();
    if(now >= (m_lastTime + 3)) {
        FSUtil::MakeSurePathExist(m_filename);
        ReOpen();
        if(m_reopenError) {
            std::cout << "reopen file " << m_filename << " error" << std::endl;
        }
        m_lastTime = now;
    }

    if(m_reopenError) {
        return;
    }

    MutexType::Lock lock(m_mutex);
    if(m_formatter) {
         if(!m_formatter->Format(m_filestream, event)) {
             std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
         }
    } else {
         if(!m_defaultFormatter->Format(m_filestream, event)) {
             std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
         }
    }  
}

bool FileLogAppender::ReOpen() {
    MutexType::Lock lock(m_mutex);

    if(m_filestream) {
        m_filestream.close();
    }

    m_filestream.open(m_filename, std::ios::app);
    m_reopenError = !m_filestream;
    return !m_reopenError;
}

std::string FileLogAppender::ToYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    node["pattern"] = m_formatter ? m_formatter->GetPattern() : m_defaultFormatter->GetPattern();
    
    std::stringstream ss;
    ss << node;
    return ss.str();
}

Logger::Logger(const std::string &name)
    : m_name(name)
    , m_level(LogLevel::INFO)
    , m_createTime(GetElapsedMS()) {

}

void Logger::AddAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    m_appenders.push_back(appender);
}

void Logger::DelAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);

    for(auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::ClearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

/**
 * 调用Logger的所有appenders将日志写一遍，
 * Logger至少要有一个appender，否则没有输出
 */
void Logger::Log(LogEvent::ptr event) {
    if(event->GetLevel() <= m_level) {
        for(auto &i : m_appenders) {
            i->Log(event);
        }
    }
}

std::string Logger::ToYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);
    for(auto &i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->ToYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
    : m_logger(logger)
    , m_event(event) {
}

/**
 * @note LogEventWrap在析构时写日志
 */
LogEventWrap::~LogEventWrap() {
    m_logger->Log(m_event);
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger("root"));
    m_root->AddAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->GetName()] = m_root;
    Init();
}

/**
 * 如果指定名称的日志器未找到，那会就新创建一个，但是新创建的Logger是不带Appender的，
 * 需要手动添加Appender
 */
Logger::ptr LoggerManager::GetLogger(const std::string &name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    m_loggers[name] = logger;
    return logger;
}

/**
 * @todo 实现从配置文件加载日志配置
 */
void LoggerManager::Init() {

}

std::string LoggerManager::ToYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->ToYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

/////////////////////////////////////////////////////////////////////////////
// 从配置文件中加载日志配置
/**
 * @brief 日志输出器配置结构体定义
 */
struct LogAppenderDefine {
    int type = 0; // 1 File, 2 Stdout
    std::string pattern;
    std::string file;

    bool operator==(const LogAppenderDefine &oth) const {
        return type == oth.type && pattern == oth.pattern && file == oth.file;
    }
};

/**
 * @brief 日志器配置结构体定义
 */
struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::NOTSET;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &oth) const {
        return name == oth.name && level == oth.level && appenders == appenders;
    }

    bool operator<(const LogDefine &oth) const {
        return name < oth.name;
    }

    bool IsValid() const {
        return !name.empty();
    }
};

template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string &v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");

        if(n["appenders"].IsDefined()) {
            for(size_t i = 0; i < n["appenders"].size(); i++) {
                auto a = n["appenders"][i];
                if(!a["type"].IsDefined()) {
                    std::cout << "log appender config error: appender type is null, " << a << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1; 
                    if(!a["file"].IsDefined()) {
                        std::cout << "log appender config error: file appender file is null, " << a << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["pattern"].IsDefined()) {
                        lad.pattern = a["pattern"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(a["pattern"].IsDefined()) {
                        lad.pattern = a["pattern"].as<std::string>();
                    }
                } else {
                    std::cout << "log appender config error: appender type is invalid, " << a << std::endl;
                    continue;
                }
                ld.appenders.push_back(lad);
            }
        } // end for
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine &i) {
        YAML::Node n;
        n["name"] = i.name;
        n["level"] = LogLevel::ToString(i.level);
        for(auto &a : i.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if(!a.pattern.empty()) {
                na["pattern"] = a.pattern;
            }
            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

ConfigVar<std::set<LogDefine>>::ptr g_log_defines = 
    Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->AddListener([](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value){
            LOG_INFO(LOG_ROOT()) << "on log config changed";
            for(auto &i : new_value) {
                auto it = old_value.find(i);
                Logger::ptr logger;
                if(it == old_value.end()) {
                    // 新增logger
                    logger = LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        // 修改的logger
                        logger = LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->SetLevel(i.level);
                logger->ClearAppenders();
                for(auto &a : i.appenders) {
                    zch::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new zch::FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new zch::StdoutLogAppender);
                    }
                    if(!a.pattern.empty()) {
                        ap->SetFormatter(LogFormatter::ptr(new LogFormatter(a.pattern)));
                    } else {
                        ap->SetFormatter(LogFormatter::ptr(new LogFormatter));
                    }
                    logger->AddAppender(ap);
                }
            }

            // 以配置文件为主，如果程序里定义了配置文件中未定义的logger，那么把程序里定义的logger设置成无效
            for(auto &i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    auto logger = LOG_NAME(i.name);
                    logger->SetLevel(LogLevel::NOTSET);
                    logger->ClearAppenders();
                }
            }
        });
    }
};

// 在 main 函数之前注册配置更改的回调函数
// 用于在更新配置时将 log 相关的配置加载到 Config
static LogIniter __log_init;

}
