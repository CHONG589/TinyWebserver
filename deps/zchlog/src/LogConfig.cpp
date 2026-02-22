#include "LogConfig.h"
#include "Logger.h"
#include "LogLevel.hpp"
#include "LogSink.h"
#include "Log.h"

#include <fstream>
#include <json/json.h>
#include <sstream>

/**
 * @brief 解析日志级别
 * @param[in] level_str 日志级别字符串
 * @return zch::LogLevel::Level 日志级别
 */
static zch::LogLevel::Level ParseLevel(const std::string& level_str) {
    if (level_str == "DEBUG") return zch::LogLevel::Level::DEBUG;
    if (level_str == "INFO")  return zch::LogLevel::Level::INFO;
    if (level_str == "WARN")  return zch::LogLevel::Level::WARN;
    if (level_str == "ERROR") return zch::LogLevel::Level::ERROR;
    if (level_str == "FATAL") return zch::LogLevel::Level::FATAL;

    return zch::LogLevel::Level::DEBUG;
}

/**
 * @brief 解析日志器类型
 * @param[in] type_str 日志器类型字符串
 * @return zch::LoggerType 日志器类型
 */
static zch::LoggerType ParseLoggerType(const std::string& type_str) {
    if (type_str == "async") {
        return zch::LoggerType::Async_Logger;
    } else {
        return zch::LoggerType::Sync_Logger;
    } 
}

/**
 * @brief 从 JSON 文件初始化日志配置
 * @param[in] path JSON 文件路径
 * @return void
 */
void zch::InitLogFromJson(const std::string& path) {
    Json::Value root;
    Json::String err;
    Json::CharReaderBuilder rbuilder;

    // 打开配置文件（失败则终止）
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "cannot open log config file: " << path << std::endl;
        std::abort();
    }

    // 解析 JSON（失败则终止）
    if (!Json::parseFromStream(rbuilder, ifs, &root, &err)) {
        std::cerr << "parse log config json error: " << err << std::endl;
        std::abort();
    }
    
    const Json::Value& loggers = root["loggers"];
    // 遍历 loggers，构建每个日志器
    for (Json::ArrayIndex i = 0; i < loggers.size(); ++i) {
        const Json::Value& cfg = loggers[i];

        // 使用全局建造者，构建完成后自动注册到 LogManager
        std::unique_ptr<zch::LoggerBuilder> builder(new zch::GlobalLoggerBuilder());      

        // 设置日志名称
        std::string name = cfg.get("name", "").asString();
        builder->BuildName(name);

        // 设置日志等级
        std::string level_str = cfg.get("level", "DEBUG").asString();
        builder->BuildLevel(ParseLevel(level_str));

        // 设置日志类型：sync 同步，async 异步
        std::string type_str = cfg.get("type", "sync").asString();
        builder->BuildType(ParseLoggerType(type_str));

        // 异步不安全模式（更快但不保证时序与完整性）
        bool async_unsafe = cfg.get("async_unsafe", false).asBool();
        if (async_unsafe) {
            builder->BuildEnableUnSafe();
        }

        // 格式化规则（若未设置，内部会采用默认规则）
        std::string pattern = cfg.get("pattern", "[%d{%Y-%m-%d %H:%M:%S}][%p][%f:%l]%m%n").asString();
        if (!pattern.empty()) {
            builder->BuildFormatter(pattern);
        }

        // 设置落地方向 sinks：
        // - stdout：标准输出
        // - roll_by_size：按大小滚动；支持 dir 指定输出目录（不存在则创建）
        // - roll_by_time: 按时间滚动；支持 dir 指定输出目录（不存在则创建），gap 指定滚动间隔（hour/day）
        const Json::Value& sinks = cfg["sinks"];
        for (Json::ArrayIndex j = 0; j < sinks.size(); ++j) {
            const Json::Value& sink_cfg = sinks[j];
            std::string sink_type = sink_cfg.get("type", "stdout").asString();
            if (sink_type == "stdout") {
                builder->AddLogSink<zch::StdOutSink>();
            } else if (sink_type == "file") {
                std::string file_path = sink_cfg.get("path", "app.log").asString();
                builder->AddLogSink<zch::FileSink>(file_path);
            } else if (sink_type == "roll_by_size") {
                std::uint64_t max_size = sink_cfg.get("max_size", 1024 * 1024).asUInt64();
                std::string dir = sink_cfg.get("dir", "logs").asString();
                if (!dir.empty()) {
                    builder->AddLogSink<zch::RollBySizeSink>(dir, max_size);
                } else {
                    builder->AddLogSink<zch::RollBySizeSink>(max_size);
                }
            } else if (sink_type == "roll_by_time") {
                std::string dir = sink_cfg.get("dir", "logs").asString();
                std::string gap_str = sink_cfg.get("gap", "day").asString();
                zch::TimeGap gap = (gap_str == "hour") ? zch::TimeGap::GAP_HOUR : zch::TimeGap::GAP_DAY;
                builder->AddLogSink<zch::RollByTimeSink>(dir, gap);
            }
        }

        zch::Logger::ptr logger = builder->Build();
        // 将变量强制转换为 void 类型。这在 C++ 中是一个不做
        // 任何操作的语句（No-op），但它告诉编译器：“我知道这个
        // 变量存在，我是故意不使用它的，请不要报错。”
        (void)logger;
    }
}
