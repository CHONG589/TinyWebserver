/**
 * @file Log.h
 * @brief 日志宏
 * @author zch
 * @date 2025-11-15
 */

#ifndef LOG_H__
#define LOG_H__

#include <string.h>

#include "Logger.h"
#include "LogLevel.hpp"
#include "Formatter.h"

namespace zch {

    // __FILENAME__ 只显示文件名，__FILE__ 显示的是该文件的路径
    inline const char* __filename_impl(const char* path) {
        const char* p1 = strrchr(path, '/');
        const char* p2 = strrchr(path, '\\');
        const char* p = (p1 && p2) ? (p1 > p2 ? p1 : p2) : (p1 ? p1 : p2);
        return p ? p + 1 : path;
    }

    #define __FILENAME__ ::zch::__filename_impl(__FILE__)

    // 流式日志类
    class LogStream {
    public:
        LogStream(zch::Logger::ptr logger, zch::LogLevel::Level level, const char* file, size_t line) 
                    : logger_(logger), level_(level), file_(file), line_(line) {}

        ~LogStream() {
            if(level_ >= logger_->GetLimitLevel()) {
                zch::LogMsg msg(level_, logger_->GetLoggerName().c_str(), file_, line_, stream_.str());
                std::string logMessage = logger_->GetFormatter()->Format(msg);
                logger_->log(logMessage.c_str(), logMessage.size());
            }
        }
        
        // 重载 << 操作符
        template<typename T>
        LogStream& operator<<(const T& value) {
            stream_ << value;
            return *this;
        }
        
        // 处理 std::endl 等操作符
        LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
            stream_ << manip;
            return *this;
        }
        
    private:
        zch::Logger::ptr logger_;
        zch::LogLevel::Level level_;
        const char* file_;
        size_t line_;
        std::ostringstream stream_;
    };

    // 基于日志器的日志宏
    #define LOG_DEBUG_TO(logger) zch::LogStream(logger, zch::LogLevel::Level::DEBUG, __FILENAME__, __LINE__)
    #define LOG_INFO_TO(logger) zch::LogStream(logger, zch::LogLevel::Level::INFO, __FILENAME__, __LINE__)
    #define LOG_WARN_TO(logger) zch::LogStream(logger, zch::LogLevel::Level::WARN, __FILENAME__, __LINE__)
    #define LOG_ERROR_TO(logger) zch::LogStream(logger, zch::LogLevel::Level::ERROR, __FILENAME__, __LINE__)
    #define LOG_FATAL_TO(logger) zch::LogStream(logger, zch::LogLevel::Level::FATAL, __FILENAME__, __LINE__)

    // 默认使用 root 日志器的日志宏
    #define LOG_DEBUG() LOG_DEBUG_TO(zch::LogManager::GetInstance().GetRoot())
    #define LOG_INFO() LOG_INFO_TO(zch::LogManager::GetInstance().GetRoot())
    #define LOG_WARN() LOG_WARN_TO(zch::LogManager::GetInstance().GetRoot())
    #define LOG_ERROR() LOG_ERROR_TO(zch::LogManager::GetInstance().GetRoot())
    #define LOG_FATAL() LOG_FATAL_TO(zch::LogManager::GetInstance().GetRoot())

    // 基于日志器名称的日志宏
    #define LOG_DEBUG_BY_NAME(name) \
        if (auto __logger = zch::LogManager::GetInstance().GetLogger(name)) \
            zch::LogStream(__logger, zch::LogLevel::Level::DEBUG, __FILENAME__, __LINE__)

    #define LOG_INFO_BY_NAME(name) \
        if (auto __logger = zch::LogManager::GetInstance().GetLogger(name)) \
            zch::LogStream(__logger, zch::LogLevel::Level::INFO, __FILENAME__, __LINE__)

    #define LOG_WARN_BY_NAME(name) \
        if (auto __logger = zch::LogManager::GetInstance().GetLogger(name)) \
            zch::LogStream(__logger, zch::LogLevel::Level::WARN, __FILENAME__, __LINE__)

    #define LOG_ERROR_BY_NAME(name) \
        if (auto __logger = zch::LogManager::GetInstance().GetLogger(name)) \
            zch::LogStream(__logger, zch::LogLevel::Level::ERROR, __FILENAME__, __LINE__)

    #define LOG_FATAL_BY_NAME(name) \
        if (auto __logger = zch::LogManager::GetInstance().GetLogger(name)) \
            zch::LogStream(__logger, zch::LogLevel::Level::FATAL, __FILENAME__, __LINE__)
}

#endif
