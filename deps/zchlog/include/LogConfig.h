/**
 * @file LogConfig.h
 * @brief 日志配置文件
 * @author zch
 * @date 2026-01-18
 */

#ifndef LOG_CONFIG_H__
#define LOG_CONFIG_H__

#include <string>

#include "LogConfig.h"
#include "Logger.h"
#include "LogLevel.hpp"
#include "LogSink.h"
#include "Log.h"
//#include "../../../include/base/config.h"

namespace zch {
    /**
    * @brief 从 JSON 文件初始化日志配置
    * @param[in] path JSON 文件路径
    * @return void
    */
    void InitLogFromJson(const std::string& path);

    // struct LogAppenderDefine {
    //     int type = 0;   // 1=stdout, 2=file, 3=roll_by_size, 4=roll_by_time
    //     std::string file;
    //     std::string dir;
    //     uint64_t max_size = 0;
    //     std::string gap;     // "hour"/"day"
    //     std::string pattern; // 可选

    //     bool operator==(const LogAppenderDefine& o) const;
    // };

    // struct LogDefine {
    //     std::string name;
    //     zch::LogLevel::Level level = zch::LogLevel::Level::DEBUG;
    //     zch::LoggerType logger_type = zch::LoggerType::Sync_Logger;
    //     bool async_unsafe = false;
    //     std::string pattern;
    //     std::vector<LogAppenderDefine> appenders;

    //     bool operator==(const LogDefine& o) const;
    //     bool operator<(const LogDefine& o) const { return name < o.name; }
    // };

    // void InitLogFromConfig();
}

#endif
