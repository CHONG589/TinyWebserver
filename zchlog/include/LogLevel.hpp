/**
 * @file LogLevel.hpp
 * @brief 定义日志的等级
 * @author zch
 * @date 2025-11-02
 */

#ifndef LOGLEVEL_H__
#define LOGLEVEL_H__

#include <iostream>
#include <string>

namespace zch {

    class LogLevel {
    public:
        enum class Level {
			UNKONWN = 0,
			DEBUG,
			INFO,
			WARN,
			ERROR,
			FATAL,
			OFF
		};

        static std::string ToString(LogLevel::Level level) {
			switch (level) {
                case LogLevel::Level::DEBUG: 
                    return "DEBUG";
                case LogLevel::Level::INFO: 
                    return "INFO";
                case LogLevel::Level::WARN: 
                    return "WARN";
                case LogLevel::Level::ERROR: 
                    return "ERROR";
                case LogLevel::Level::FATAL: 
                    return "FATAL";
                case LogLevel::Level::OFF: 
                    return "OFF";
                default:
                    break;
			}
			return "UNKONWN";
		}
    };
}

 #endif
