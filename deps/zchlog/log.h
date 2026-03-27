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
#include <yaml-cpp/yaml.h>
#include <fstream>

#include "util.h"
#include "mutex.h"

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

class LogMsg {
public:
    typedef std::shared_ptr<LogMsg> ptr;

    LogMsg(LogLevel::Level level, const char* loggerName, const char* file,
			size_t line, std::string payload)
			: ctime_(Date::Now())
			, level_(level)
			, loggerName_(loggerName)
			, tid_(std::this_thread::get_id())
			, file_(file)
			, line_(line)
			, payload_(std::move(payload)) {}

    time_t GetTime() const { return ctime_; }
    LogLevel::Level GetLevel() const { return level_; }
    const char* GetLoggerName() const { return loggerName_; }
    std::thread::id GetThreadId() const { return tid_; }
    std::string GetFile() const { return file_; }
    size_t GetLine() const { return line_; }
    const std::string GetContent() const { return payload_; }

private:
    time_t ctime_;				// 时间戳
    LogLevel::Level level_;		// 日志等级
    const char* loggerName_;	// 日志器名称 (指向静态内存)
    std::thread::id tid_;		// 线程id
    const char* file_;			// 源码文件名 (指向静态内存)
    size_t line_;				// 源码行号
    std::string payload_;		// 有效载荷 (只有这里可能发生拷贝)
};

/**
 * 日志格式如：%d{%H:%M:%S}%T[%t]%T[%p]%T[%c]%T%f:%l%T%m%n
 * 
 * 日志格式符               描述
 *     %d                  日期
 *     %T                  缩进
 *     %t                 线程 id
 *     %p                 日志级别
 *     %c                日志器名称
 *     %f                  文件名
 *     %l                  行号
 *     %m                 日志消息
 *     %n                  换行
 */

// 格式化基类
class FormatItem {
public:
    using ptr = std::shared_ptr<FormatItem>;

    /** 
     * @brief 格式化操作（纯虚函数）
     * @param[in,out] out 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    virtual void Format(std::ostream& out, LogMsg::ptr msg) = 0;
};

// 日期格式化子项
class TimeFormatItem : public FormatItem {
public:
    /** 
     * @brief 构造函数
     * @param[in] fmt 时间格式化字符串(如 %Y-%m-%d %H:%M:%S)
     */
    TimeFormatItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S") : timeFmt_(fmt) {}

    /** 
     * @brief 格式化时间戳
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        struct tm t;
        char buf[32] = { 0 };
        time_t time = msg->GetTime();

        // 使用C库函数对时间戳进行格式化
        Date::LocalTime(&time, &t);
        // 将结构体 t 中存储的时间按照 _time_fmt 的格式进行存储到 buf 中。 
        strftime(buf, sizeof(buf), timeFmt_.c_str(), &t);
        // 放入流中
        oss << buf;
    }
private:
    std::string timeFmt_;
};

// 日志等级格式化子项
class LevelFormatItem : public FormatItem {
public:
    /** 
     * @brief 格式化日志等级
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << LogLevel::ToString(msg->GetLevel());
    }
};

// 日志器名称格式化子项
class LoggerNameFormatItem : public FormatItem {
public:
    /** 
     * @brief 格式化日志器名称
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << msg->GetLoggerName();
    }
};

// 线程id格式化子项
class ThreadFormatItem : public FormatItem {
public:
    /** 
     * @brief 格式化线程ID
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << msg->GetThreadId();
    }
};

// 文件名称格式化子项
class FileNameFormatItem : public FormatItem {
public:
    /** 
     * @brief 格式化文件名
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << msg->GetFile();
    }
};

// 文件行号格式化子项
class LineFormatItem : public FormatItem {
public:
    /** 
     * @brief 格式化行号
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << msg->GetLine();
    }
};

// 日志有效信息格式化子项
class MsgFormatItem : public FormatItem {
public:
    /** 
     * @brief 格式化日志载荷
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中，输出消息前面加了一个空格
        oss << " " << msg->GetContent();
    }
};

// 制表符格式化子项
class TabFormatItem : public FormatItem {
public:
    /** 
     * @brief 输出制表符
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << "\t";
    }
};

// 新行格式化子项
class NewLineFormatItem : public FormatItem {
public:
    /** 
     * @brief 输出换行符
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 提取指定字段插入流中
        oss << "\n";
    }
};

// 原始字符格式化子项
// 由于原始字符是我们想要在日志中添加的字符，在LogMsg中不存在对应的字段，所以我们需要传递参数
// 如在输出日志格式的时候，会有[%d{%H:%M:%S}][%p][%f:%l]%m%n，其中 [] 就是其它字符 
class OtherFormatItem : public FormatItem {
public:
    // 想要在日志中添加的字符
    /** 
     * @brief 构造函数
     * @param[in] str 原始字符串
     */
    OtherFormatItem(const std::string& str) :str_(str) {}

    /** 
     * @brief 输出原始字符串
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg) override {
        // 将字符串添加到流中
        oss << str_;
    }

private:
    std::string str_;
};

// 格式化器
class Formatter {
public:
    using ptr = std::shared_ptr<Formatter>;

    /** 
     * @brief 构造函数
     * @param[in] pattern 格式化模式字符串
     */
    Formatter(const std::string& pattern = "[%d{%Y-%m-%d %H:%M:%S}][%p][%f:%l]%m%n") 
            : pattern_(pattern) {
        assert(ParsePattern());
    }

    /** 
     * @brief 格式化消息并写入流
     * @param[in,out] oss 输出流
     * @param[in] msg 日志消息
     * @return void
     */
    void Format(std::ostream& oss, LogMsg::ptr msg);

    /** 
     * @brief 格式化消息并返回字符串
     * @param[in] msg 日志消息
     * @return std::string 格式化后的日志字符串
     */
    std::string Format(LogMsg::ptr msg) {
        // 复用 Format(std::ostream& oss, const LogMsg& msg) 接口
        std::stringstream oss;
        Format(oss, msg);

        return oss.str();
    }

    /**
     * @brief 获取pattern
     */
    std::string GetPattern() const { return pattern_; }

private:
    /** 
     * @brief 解析格式化模式字符串
     * @return bool 解析成功返回 true
     */
    bool ParsePattern();

    /** 
     * @brief 创建格式化子项
     * @param[in] key 格式化字符（如 d, p, m 等）
     * @param[in] val 格式化参数（如时间格式）
     * @return FormatItem::ptr 格式化子项指针
     */
    FormatItem::ptr CreateItem(const std::string& key, const std::string& val);

private:
    std::string pattern_;                       // 格式化规则字符串
	std::vector<FormatItem::ptr> items_;        // 按顺序存储指定的格式化对象
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
    LogAppender(Formatter::ptr default_formatter);
    
    /**
     * @brief 析构函数
     */
    virtual ~LogAppender() {}

    /**
     * @brief 设置日志格式器
     */
    void SetFormatter(Formatter::ptr val);

    /**
     * @brief 获取日志格式器
     */
    Formatter::ptr GetFormatter();

    /**
     * @brief 写入日志
     */
    virtual void Log(LogMsg::ptr event) = 0;

    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    virtual std::string ToYamlString() = 0;

protected:
    // Mutex
    MutexType m_mutex;
    // 日志格式器
    Formatter::ptr m_formatter;
    // 默认日志格式器
    Formatter::ptr m_defaultFormatter;
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
    void Log(LogMsg::ptr event) override;

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
    void Log(LogMsg::ptr event) override;

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

}

#endif
