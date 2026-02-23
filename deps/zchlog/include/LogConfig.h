/**
 * @file LogConfig.h
 * @brief 日志配置文件
 * @author zch
 * @date 2026-01-18
 */

#ifndef LOG_CONFIG_H__
#define LOG_CONFIG_H__

#include <string>

namespace zch {
    /**
    * @brief 从 JSON 文件初始化日志配置
    * @param[in] path JSON 文件路径
    * @return void
    */
    void InitLogFromJson(const std::string& path);

}

#endif
