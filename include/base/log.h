/**
 * @file log.h
 * @brief 日志模块
 * @date 2026-03-27
 */

#ifndef ZCH_LOG_H__
#define ZCH_LOG_H__

#include <string>
#include <thread>
#include <memory>
#include <vector>
#include <cassert>
#include <sstream>
#include <unordered_set>
#include <fstream>

#include "util.h"
#include "mutex.h"
#include "singleton.h"
#include "config.h"

// __FILENAME__ 只显示文件名，__FILE__ 显示的是该文件的路径
inline const char* __filename_impl(const char* path) {
    const char* p1 = strrchr(path, '/');
    const char* p2 = strrchr(path, '\\');
    const char* p = (p1 && p2) ? (p1 > p2 ? p1 : p2) : (p1 ? p1 : p2);
    return p ? p + 1 : path;
}

#define __FILENAME__ __filename_impl(__FILE__)

/**
 * @brief 获取root日志器
 */
#define LOG_ROOT() zch::LoggerMgr::GetInstance()->GetRoot()

/**
 * @brief 获取指定名称的日志器
 */
#define LOG_NAME(name) zch::LoggerMgr::GetInstance()->GetLogger(name)

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 * @details 构造一个LogEventWrap对象，包裹包含日志器和日志事件，在对象析构时调用日志器写日志事件
 * @todo 协程id未实现，暂时写0
 */
#define LOG_LEVEL(logger , level) \
    if(level <= logger->GetLevel()) \
        zch::LogEventWrap(logger, zch::LogEvent::ptr(new zch::LogEvent(logger->GetName(), \
            level, __FILENAME__, __LINE__, GetElapsedMS() - logger->GetCreateTime(), time(0)))).GetLogEvent()->GetSS()

#define LOG_FATAL(logger) LOG_LEVEL(logger, zch::LogLevel::FATAL)

#define LOG_ALERT(logger) LOG_LEVEL(logger, zch::LogLevel::ALERT)

#define LOG_CRIT(logger) LOG_LEVEL(logger, zch::LogLevel::CRIT)

#define LOG_ERROR(logger) LOG_LEVEL(logger, zch::LogLevel::ERROR)

#define LOG_WARN(logger) LOG_LEVEL(logger, zch::LogLevel::WARN)

#define LOG_NOTICE(logger) LOG_LEVEL(logger, zch::LogLevel::NOTICE)

#define LOG_INFO(logger) LOG_LEVEL(logger, zch::LogLevel::INFO)

#define LOG_DEBUG(logger) LOG_LEVEL(logger, zch::LogLevel::DEBUG)

namespace zch {

/**
 * @brief 日志级别
 */
class LogLevel {
public:
    enum Level { 
        // 致命情况，系统不可用
        FATAL  = 0,
        // 高优先级情况，例如数据库系统崩溃
        ALERT  = 100,
        // 严重错误，例如硬盘错误
        CRIT   = 200,
        // 错误
        ERROR  = 300,
        // 警告
        WARN   = 400,
        // 正常但值得注意
        NOTICE = 500,
        // 一般信息
        INFO   = 600,
        // 调试信息
        DEBUG  = 700,
        // 未设置
        NOTSET = 800,
    };

    /**
     * @brief 日志级别转字符串
     * @param[in] level 日志级别 
     * @return 字符串形式的日志级别
     */
    static const char *ToString(LogLevel::Level level);

    /**
     * @brief 字符串转日志级别
     * @param[in] str 字符串 
     * @return 日志级别
     * @note 不区分大小写
     */
    static LogLevel::Level FromString(const std::string &str);
};

/**
 * @brief 日志事件
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    /**
     * @brief 构造函数
     * @param[in] logger_name 日志器名称
     * @param[in] level 日志级别
     * @param[in] file 文件名
     * @param[in] line 行号
     * @param[in] elapse 从日志器创建开始到当前的累计运行毫秒
     * @param[in] time UTC时间
     */
    LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file
        , int32_t line, int64_t elapse, time_t time);

    /**
     * @brief 获取日志级别
     */
    LogLevel::Level GetLevel() const { return m_level; }

    /**
     * @brief 获取日志内容
     */
    std::string GetContent() const { return m_ss.str(); }

    /**
     * @brief 获取文件名
     */
    std::string GetFile() const { return m_file; }

    /**
     * @brief 获取行号
     */
    int32_t GetLine() const { return m_line; }

    /**
     * @brief 获取累计运行毫秒数
     */
    int64_t GetElapse() const { return m_elapse; }

    /**
     * @brief 返回时间戳
     */
    time_t GetTime() const { return m_time; }

    /**
     * @brief 获取内容字节流，用于流式写入日志
     */
    std::stringstream &GetSS() { return m_ss; }

    /**
     * @brief 获取日志器名称
     */
    const std::string &GetLoggerName() const { return m_loggerName; }

private:
    // 日志级别
    LogLevel::Level m_level;
    // 日志内容，使用stringstream存储，便于流式写入日志
    std::stringstream m_ss;
    // 文件名
    const char *m_file = nullptr;
    // 行号
    int32_t m_line = 0;
    // 从日志器创建开始到当前的耗时
    int64_t m_elapse = 0;
    // UTC时间戳
    time_t m_time;
    // 日志器名称
    std::string m_loggerName;
};

/**
 * @brief 日志格式化
 */
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 模板参数说明：
     * - %%m 消息
     * - %%p 日志级别
     * - %%c 日志器名称
     * - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
     * - %%r 该日志器创建后的累计运行毫秒数
     * - %%f 文件名
     * - %%l 行号
     * - %%t 线程id
     * - %%F 协程id
     * - %%N 线程名称
     * - %%% 百分号
     * - %%T 制表符
     * - %%n 换行
     * 
     * 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n
     * 
     * 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
     */
    LogFormatter(const std::string &pattern = "[%d{%Y-%m-%d %H:%M:%S}][%rms][%p][%c][%f:%l] %m%n");

    /**
     * @brief 初始化，解析格式模板，提取模板项
     */
    void Init();

    /**
     * @brief 模板解析是否出错
     */
    bool IsError() const { return m_error; }

    /**
     * @brief 对日志事件进行格式化，返回格式化日志文本
     * @param[in] event 日志事件
     * @return 格式化日志字符串
     */
    std::string Format(LogEvent::ptr event);

    /**
     * @brief 对日志事件进行格式化，返回格式化日志流
     * @param[in] event 日志事件
     * @param[in] os 日志输出流
     * @return 格式化日志流
     */
    std::ostream &Format(std::ostream &os, LogEvent::ptr event);

    /**
     * @brief 获取pattern
     */
    std::string GetPattern() const { return m_pattern; }

public:
    /**
     * @brief 日志内容格式化项，虚基类，用于派生出不同的格式化项
     */
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        
        /**
         * @brief 析构函数
         */
        virtual ~FormatItem() {}

        /**
         * @brief 格式化日志事件
         */
        virtual void Format(std::ostream &os, LogEvent::ptr event) = 0;
    };

private:
    // 日志格式模板
    std::string m_pattern;
    // 解析后的格式模板数组
    std::vector<FormatItem::ptr> m_items;
    // 是否出错
    bool m_error = false;
};

/**
 * @brief 日志输出地，虚基类，用于派生出不同的LogAppender
 * @details 参考log4cpp，Appender自带一个默认的LogFormatter，以控件默认输出格式
 */
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief 构造函数
     * @param[in] default_formatter 默认日志格式器
     */
    LogAppender(LogFormatter::ptr default_formatter);
    
    /**
     * @brief 析构函数
     */
    virtual ~LogAppender() {}

    /**
     * @brief 设置日志格式器
     */
    void SetFormatter(LogFormatter::ptr val);

    /**
     * @brief 获取日志格式器
     */
    LogFormatter::ptr GetFormatter();

    /**
     * @brief 写入日志
     */
    virtual void Log(LogEvent::ptr event) = 0;

    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    virtual std::string ToYamlString() = 0;

protected:
    // Mutex
    MutexType m_mutex;
    // 日志格式器
    LogFormatter::ptr m_formatter;
    // 默认日志格式器
    LogFormatter::ptr m_defaultFormatter;
};

/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    /**
     * @brief 构造函数
     */
    StdoutLogAppender();

    /**
     * @brief 写入日志
     */
    void Log(LogEvent::ptr event) override;

    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    std::string ToYamlString() override;
};

/**
 * @brief 输出到文件
 */
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    /**
     * @brief 构造函数
     * @param[in] file 日志文件路径
     */
    FileLogAppender(const std::string &file);

    /**
     * @brief 写日志
     */
    void Log(LogEvent::ptr event) override;

    /**
     * @brief 重新打开日志文件
     * @return 成功返回true
     */
    bool ReOpen();

    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    std::string ToYamlString() override;

private:
    // 文件路径
    std::string m_filename;
    // 文件流
    std::ofstream m_filestream;
    // 上次重打打开时间
    uint64_t m_lastTime = 0;
    // 文件打开错误标识
    bool m_reopenError = false;
};

/**
 * @brief 日志器类
 * @note 日志器类不带root logger
 */
class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief 构造函数
     * @param[in] name 日志器名称 
     */
    Logger(const std::string &name="default");

    /**
     * @brief 获取日志器名称
     */
    const std::string &GetName() const { return m_name; }

    /**
     * @brief 获取创建时间
     */
    const uint64_t &GetCreateTime() const { return m_createTime; }

    /**
     * @brief 设置日志级别
     */
    void SetLevel(LogLevel::Level level) { m_level = level; }

    /**
     * @brief 设置日志级别
     */
    void GetLevel(LogLevel::Level level) { m_level = level; }

    /**
     * @brief 获取日志级别
     */
    LogLevel::Level GetLevel() const { return m_level; }

    /**
     * @brief 添加LogAppender
     */
    void AddAppender(LogAppender::ptr appender);

    /**
     * @brief 删除LogAppender
     */
    void DelAppender(LogAppender::ptr appender);

    /**
     * @brief 清空LogAppender
     */
    void ClearAppenders();

    /**
     * @brief 写日志
     */
    void Log(LogEvent::ptr event);

    /**
     * @brief 将日志器的配置转成YAML String
     */
    std::string ToYamlString();

private:
    // Mutex
    MutexType m_mutex;
    // 日志器名称
    std::string m_name;
    // 日志器等级
    LogLevel::Level m_level;
    // LogAppender集合
    std::list<LogAppender::ptr> m_appenders;
    // 创建时间（毫秒）
    uint64_t m_createTime;
};

/**
 * @brief 日志事件包装器，方便宏定义，内部包含日志事件和日志器
 */
class LogEventWrap{
public:
    /**
     * @brief 构造函数
     * @param[in] logger 日志器 
     * @param[in] event 日志事件
     */
    LogEventWrap(Logger::ptr logger, LogEvent::ptr event);

    /**
     * @brief 析构函数
     * @details 日志事件在析构时由日志器进行输出
     */
    ~LogEventWrap();

    /**
     * @brief 获取日志事件
     */
    LogEvent::ptr GetLogEvent() const { return m_event; }

private:
    // 日志器
    Logger::ptr m_logger;
    // 日志事件
    LogEvent::ptr m_event;
};

/**
 * @brief 日志器管理类
 */
class LoggerManager{
public:
    typedef Spinlock MutexType;

    /**
     * @brief 构造函数
     */
    LoggerManager();

    /**
     * @brief 初始化，主要是结合配置模块实现日志模块初始化
     */
    void Init();

    /**
     * @brief 获取指定名称的日志器
     */
    Logger::ptr GetLogger(const std::string &name);

    /**
     * @brief 获取root日志器，等效于GetLogger("root")
     */
    Logger::ptr GetRoot() { return m_root; }

    /**
     * @brief 将所有的日志器配置转成YAML String
     */
    std::string ToYamlString();

private:
    // Mutex
    MutexType m_mutex;
    // 日志器集合
    std::map<std::string, Logger::ptr> m_loggers;
    // root日志器
    Logger::ptr m_root;
};

// 日志器管理类单例
typedef Singleton<LoggerManager> LoggerMgr;

}

#endif
