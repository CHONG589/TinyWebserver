#include "Logger.h"

// 用于直接将数据写入日志器的落地方向，不进行格式化，用来输出图案
/** 
 * @brief 原始日志写入接口（不经格式化）
 * @param[in] data 日志数据
 * @param[in] len 数据长度
 * @return void
 */
void zch::Logger::WriteRaw(const char* data, size_t len) {
    std::vector<zch::LogSink::ptr> sinks_copy;
    {
        std::unique_lock<std::mutex> ulk(mtx_);
        sinks_copy = sinks_;
    }
    
    for (auto& sink : sinks_copy) {
        if (sink.get() != nullptr) {
            sink->log(data, len);
        }
    }
}

/** 
 * @brief 同步日志写入
 * @param[in] data 日志数据
 * @param[in] len 数据长度
 * @return void
 */
void zch::SyncLogger::log(const char* data, size_t len) {
	// 锁只在复制 sinks_ 时使用，减少锁的颗粒度，复制完后就解锁。
	// 前提是 sinks_ 不会在 log() 中被修改，只有在 AddLogSink() 中被修改。
	std::vector<zch::LogSink::ptr> sinks_copy;
	{
		std::unique_lock<std::mutex> ulk(mtx_);
		sinks_copy = sinks_;
	}

	for (auto& sink : sinks_copy) {
		if (sink.get() != nullptr) {
			sink->log(data, len);
		}
	}
}

/** 
 * @brief 实际落地执行函数（由异步线程调用）
 * @param[in] buf 缓冲区数据引用
 * @return void
 */
void zch::AsyncLogger::RealSink(Buffer& buf) {
	// 异步线程根据落地方向进行数据落地
	for (auto& sink : sinks_) {
		if (sink.get() != nullptr) {
			sink->log(buf.Start(), buf.ReadableSize());
		}
	}
}

/** 
 * @brief 构建日志器实现（内部调用）
 * @param[in] isGlobalRegister 是否注册到全局管理器
 * @return zch::Logger::ptr 构建完成的日志器指针
 */
zch::Logger::ptr zch::LoggerBuilder::BuildImpl(bool isGlobalRegister) {
	// 不能没有日志器名称
	assert(!loggerName_.empty());

	// 如果用户没有手动设置过格式化器，就进行构造一个默认的格式化器
	if (formatter_.get() == nullptr) {
		formatter_ = std::make_shared<zch::Formatter>();
	}

	// 如果用户没有手动设置过落地方向数组，就进行默认设置一个的落地到标准输出的格式化器
	if (sinks_.empty()) {
		sinks_.push_back(SinkFactory::create<StdOutSink>());
	}

	// 根据日志器的类型构造相应类型的日志器
	Logger::ptr logger;
	if (loggerType_ == LoggerType::Async_Logger) {
		logger =  std::make_shared<zch::AsyncLogger>(loggerName_, limitLevel_, formatter_, sinks_, asyncType_);
	} else {
		logger =  std::make_shared<zch::SyncLogger>(loggerName_, limitLevel_, formatter_, sinks_);
	}

	if(isGlobalRegister) {
		LogManager::GetInstance().AddLogger(logger);
	}

	return logger;
}

/** 
 * @brief 构建局部日志器（不自动注册）
 * @return Logger::ptr 日志器指针
 */
zch::Logger::ptr zch::LocalLoggerBuilder::Build() {
	return BuildImpl(false);
}

/** 
 * @brief 构建全局日志器（自动注册到 LogManager）
 * @return Logger::ptr 日志器指针
 */
zch::Logger::ptr zch::GlobalLoggerBuilder::Build() {
	return BuildImpl(true);
}
