/**
 * @file LogMsg.h
 * @brief 日志消息类
 * @author zch
 * @date 2025-11-02
 */

#ifndef LOGMSG_H__
#define LOGMSG_H__

#include <ctime>
#include <string>
#include <thread>

#include "LogLevel.hpp"
#include "util.hpp"

namespace zch {

    struct LogMsg {
		time_t ctime_;				// 时间戳
		LogLevel::Level level_;		// 日志等级
		const char* logger_;		// 日志器名称 (指向静态内存)
		std::thread::id tid_;		// 线程id
		const char* file_;			// 源码文件名 (指向静态内存)
		size_t line_;				// 源码行号
		std::string payload_;		// 有效载荷 (只有这里可能发生拷贝)
		LogMsg() {}

		LogMsg(LogLevel::Level level, const char* logger, const char* file,
			size_t line, std::string payload)
			: ctime_(Date::Now())
			, level_(level)
			, logger_(logger)
			, tid_(std::this_thread::get_id())
			, file_(file)
			, line_(line)
			, payload_(std::move(payload)) {}
	};
}

#endif
