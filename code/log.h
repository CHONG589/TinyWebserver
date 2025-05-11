/**
 * @file log.h
 * @brief 日志模块
 * @date 2025-05-11
 */

#ifndef ZCH_LOG_H__
#define ZCH_LOG_H__

#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstdarg>
#include <list>
#include <map>

/**
 * @brief 日志级别
 */
class LogLevel {
public:
    enum Level { 
        FATAL  = 0,     // 致命情况，系统不可用
        ERROR  = 100,   // 错误
        WARN   = 200,   // 警告
        INFO   = 300,   // 一般信息
        DEBUG  = 400,   // 调试信息
    };
};

/**
 * @brief 日志事件
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file, int32_t line
        , int64_t elapse, uint32_t thread_id, uint64_t fiber_id, time_t time, const std::string &thread_name);

    LogLevel::Level getLevel() const { return m_level; }
    std::string getContent() const { return m_ss.str(); }
    std::string getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    int64_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint64_t getFiberId() const { return m_fiberId; }
    time_t getTime() const { return m_time; }
    const std::string &getThreadName() const { return m_threadName; }
    std::stringstream &getSS() { return m_ss; }
    const std::string &getLoggerName() const { return m_loggerName; }

    //C prinf风格写入日志
    void printf(const char *fmt, ...);
    //C vprintf风格写入日志
    void vprintf(const char *fmt, va_list ap);

private:
    LogLevel::Level m_level;            // 日志级别
    std::stringstream m_ss;             // 日志内容，使用stringstream存储，便于流式写入日志
    const char *m_file = nullptr;       // 文件名
    int32_t m_line = 0;                 // 行号
    int64_t m_elapse = 0;               // 从日志器创建开始到当前的耗时
    uint32_t m_threadId = 0;            // 线程id
    uint64_t m_fiberId = 0;             // 协程id
    time_t m_time;                      // 时间戳
    std::string m_threadName;           // 线程名称
    std::string m_loggerName;           // 日志器名称
};

/**
 * @brief 日志输出地，虚基类，用于派生出不同的LogAppender
 */
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    LogAppender(LogFormatter::ptr default_formatter);
    virtual ~LogAppender() {}

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

    //写入日志
    virtual void log(LogEvent::ptr event) = 0;

    //将日志输出目标的配置转成YAML String
    virtual std::string toYamlString() = 0;

protected:
    MutexType m_mutex;
    LogFormatter::ptr m_formatter;          // 日志格式器
    LogFormatter::ptr m_defaultFormatter;   // 默认日志格式器
};

/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    StdoutLogAppender();

    //写入日志
    void log(LogEvent::ptr event) override;

    //将日志输出目标的配置转成YAML String
    std::string toYamlString() override;
};

/**
 * @brief 输出到文件
 */
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    //file 日志文件路径
    FileLogAppender(const std::string &file);
    //写日志
    void log(LogEvent::ptr event) override;
    bool reopen();

    //将日志输出目标的配置转成YAML String
    std::string toYamlString() override;

private:
    std::string m_filename;         // 文件路径   
    std::ofstream m_filestream;     // 文件流   
    uint64_t m_lastTime = 0;        // 上次重打打开时间   
    bool m_reopenError = false;     // 文件打开错误标识
};

/**
 * @brief 日志器类
 * @note 日志器类不带root logger
 */
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string &name="default");
    const std::string &getName() const { return m_name; }
    const uint64_t &getCreateTime() const { return m_createTime; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    LogLevel::Level getLevel() const { return m_level; }
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    //写日志
    void log(LogEvent::ptr event);
    //将日志器的配置转成YAML String
    std::string toYamlString();

private:   
    MutexType m_mutex;                          // Mutex
    std::string m_name;                         // 日志器名称  
    LogLevel::Level m_level;                    // 日志器等级  
    std::list<LogAppender::ptr> m_appenders;    // LogAppender集合  
    uint64_t m_createTime;                      // 日志器创建时间
};

#endif
