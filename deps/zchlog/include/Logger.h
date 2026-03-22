/**
 * @file Logger.h
 * @brief 日志器模块：1. 完成对日志落地模块，格式化模块，日志消息模块的封装；2. 日志器模块能够完成不同的落地方式，因此根据基类日志器派生出同步日志器和异步日志器
 * @author zch
 * @date 2025-11-09
 */

#ifndef LOGGER_H__
#define LOGGER_H__

#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstdarg>

#include "LogLevel.hpp"
#include "Formatter.h"
#include "LogSink.h"
#include "AsynLopper.h"

namespace zch {

    class Logger {
    public:
        using ptr = std::shared_ptr<Logger>;
        /** 
         * @brief 构造函数
         * @param[in] loggerName 日志器名称
         * @param[in] level 日志限制等级
         * @param[in] formatter 日志格式化器
         * @param[in] sinks 日志落地方向集合
         */
        Logger(const std::string loggerName
                , zch::LogLevel::Level level
                , zch::Formatter::ptr formatter
                , std::vector<zch::LogSink::ptr> sinks)
			    : loggerName_(loggerName)
                , limitLevel_(level)
                , formatter_(formatter)
                , sinks_(sinks) {}

        /** 
         * @brief 获取日志器名称
         * @return const std::string& 日志器名称
         */
        const std::string& GetLoggerName() { return loggerName_; }

        /** 
         * @brief 获取日志格式化器
         * @return Formatter::ptr 日志格式化器
         */
		Formatter::ptr GetFormatter() { return formatter_; }

        /** 
         * @brief 获取日志限制等级
         * @return zch::LogLevel::Level 日志限制等级
         */
		zch::LogLevel::Level GetLimitLevel() { return limitLevel_; }

        /** 
         * @brief 抽象日志写入接口
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
		virtual void log(const char* data, size_t len) = 0;
        
        /** 
         * @brief 原始日志写入接口（不经格式化）
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
        void WriteRaw(const char* data, size_t len);

    protected:
        // 保护日志落地的锁
        std::mutex mtx_;
        // 日志器名称
        std::string loggerName_;
        // 日志限制等级
        zch::LogLevel::Level limitLevel_;
        // 格式化器
        zch::Formatter::ptr formatter_;
        // 落地方向集合
        std::vector<zch::LogSink::ptr> sinks_; 
    };

    // 同步日志器
    class SyncLogger : public Logger {
    public:
        /** 
         * @brief 构造函数
         * @param[in] loggerName 日志器名称
         * @param[in] level 日志限制等级
         * @param[in] formatter 日志格式化器
         * @param[in] sinks 日志落地方向集合
         */
        SyncLogger(const std::string loggerName
                    , zch::LogLevel::Level level
                    , zch::Formatter::ptr formatter
                    , std::vector<zch::LogSink::ptr> sinks)
			        : Logger(loggerName, level, formatter, sinks) {}

        /** 
         * @brief 同步日志写入
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
        void log(const char* data, size_t len) override;  
    };

    // 异步日志器
    class AsyncLogger : public Logger {
	public:
        /** 
         * @brief 构造函数
         * @param[in] loggerName 日志器名称
         * @param[in] level 日志限制等级
         * @param[in] formatter 日志格式化器
         * @param[in] sinks 日志落地方向集合
         * @param[in] type 异步写入类型（安全/非安全）
         */
		AsyncLogger(const std::string loggerName
                    , zch::LogLevel::Level level
                    , zch::Formatter::ptr formatter
                    , std::vector<zch::LogSink::ptr> sinks
                    , ASYNCTYPE type)
			        : Logger(loggerName, level, formatter, sinks)
			        , asyLopper_(std::bind(&AsyncLogger::RealSink, this, std::placeholders::_1), type) {}
    
        /** 
         * @brief 异步日志写入（推入缓冲区）
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
		void log(const char* data, size_t len) override {
			// 将数据放入异步缓冲区(这个接口是线程安全的因此不需要加锁)
			asyLopper_.Push(data, len);
		}

		/** 
         * @brief 实际落地执行函数（由异步线程调用）
         * @param[in] buf 缓冲区数据引用
         * @return void
         */
		void RealSink(Buffer& buf);

	protected:
		// 异步工作器
		AsynLopper asyLopper_;
	};

    // 使用建造者模式建造日志器，简化日志器的构建，降低用户的使用复杂度定义一个建造
    // 者基类：
	// 类里面 build() 函数会根据用户传入的类型来进行构建同步日志器或异步日志器
	// 根据基类派生出子类建造类，通过不同的子类分别构建：局部日志器和全局日志器

	// 日志器类型，根据传入的类型不同来决定是构造同步日志器，还是异步日志器
    enum class LoggerType {
		// 同步日志器
		Sync_Logger,
		// 异步日志器
		Async_Logger
	};

    class LoggerBuilder {
	public:
        /** 
         * @brief 构造函数，默认初始化各参数
         */
		LoggerBuilder() : asyncType_(ASYNCTYPE::ASYNC_SAFE)
			            , loggerType_(LoggerType::Sync_Logger)
			            , limitLevel_(LogLevel::Level::DEBUG) {}

		/** 
         * @brief 开启非安全模式（异步日志用）
         * @return void
         */
		void BuildEnableUnSafe() { asyncType_ = ASYNCTYPE::ASYNC_UN_SAFE; }

		/** 
         * @brief 设置日志器类型
         * @param[in] loggerType 日志器类型（同步/异步）
         * @return void
         */
		void BuildType(LoggerType loggerType = LoggerType::Sync_Logger) { loggerType_ = loggerType; }

		/** 
         * @brief 设置日志器名称
         * @param[in] loggerName 日志器名称
         * @return void
         */
		void BuildName(const std::string& loggerName) { loggerName_ = loggerName; }

		/** 
         * @brief 设置日志限制等级
         * @param[in] limitLevel 限制等级
         * @return void
         */
		void BuildLevel(LogLevel::Level limitLevel) { limitLevel_ = limitLevel; }

		/** 
         * @brief 设置格式化器模式字符串
         * @param[in] pattern 格式化字符串
         * @return void
         */
		void BuildFormatter(const std::string& pattern = "[%d{%Y-%m-%d %H:%M:%S}][%p][%f:%l]%m%n") {
			formatter_ = std::make_shared<zch::Formatter>(pattern);
		}

        /** 
         * @brief 构建日志器实现（内部调用）
         * @param[in] isGlobalRegister 是否注册到全局管理器
         * @return zch::Logger::ptr 构建完成的日志器指针
         */
		zch::Logger::ptr BuildImpl(bool isGlobalRegister);

		/** 
         * @brief 添加日志落地方向
         * @tparam SinkType 落地方向类型（如 FileSink）
         * @tparam Args 构造参数类型包
         * @param[in] args 构造参数
         * @return void
         */
		template<class SinkType, class ...Args>
		void AddLogSink(Args&&... args) {
			LogSink::ptr sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
			sinks_.push_back(sink);
		}

		/** 
         * @brief 构建日志器（纯虚函数）
         * @return Logger::ptr 日志器指针
         */
		virtual Logger::ptr Build() = 0;

	protected:
		// 异步日志器的写入是否开启非安全模式
		ASYNCTYPE  asyncType_;
		// 日志器的类型，同步 or 异步
		LoggerType loggerType_;
		// 日志器的名称 (每一个日志器的唯一标识)
		std::string loggerName_;
		// 日志限制输出等级
		zch::LogLevel::Level limitLevel_;
		// 格式化器
		zch::Formatter::ptr	formatter_;
		// 日志落地方向数组
		std::vector<zch::LogSink::ptr> sinks_;
	};

    // 局部日志器建造者
	class LocalLoggerBuilder : public LoggerBuilder {
	public:
        /** 
         * @brief 构建局部日志器（不自动注册）
         * @return Logger::ptr 日志器指针
         */
		Logger::ptr Build() override;
	};

	// 全局建造者,通过全局建造者建造出的对象会自动添加到 LogManager 对象中
	class GlobalLoggerBuilder : public LoggerBuilder {
	public:
        /** 
         * @brief 构建全局日志器（自动注册到 LogManager）
         * @return Logger::ptr 日志器指针
         */
		Logger::ptr Build() override;
	};

    // 日志器管理者
	class LogManager {
	public:
		/** 
         * @brief 获取 LogManager 单例
         * @return LogManager& 单例引用
         */
		static LogManager& GetInstance() {
			static LogManager ins;
			return ins;
		}

		/** 
         * @brief 添加日志器到管理器
         * @param[in] logger 日志器智能指针
         * @return void
         */
		void AddLogger(const Logger::ptr& logger) {
			// 不使用 HasLogger() 函数是因为 HasLogger() 函数中也需要加锁，
			// 而 AddLogger() 函数中已经加锁了，因此在 AddLogger() 函数中判断是否存在
			// 日志器可以避免重复添加日志器。
			std::unique_lock<std::mutex> ulk(mtxLogger_);
			const std::string& name = logger->GetLoggerName();
			auto it = loggers_.find(name);
			if(it != loggers_.end()) {
				return;
			}
			loggers_[name] = logger;
		}

		/** 
         * @brief 判断日志器是否存在
         * @param[in] loggerName 日志器名称
         * @return bool 存在返回 true，否则 false
         */
		// 适合外部调用，内部的函数不调用，形成单向依赖
		bool HasLogger(const std::string& loggerName) {
			std::unique_lock<std::mutex> ulk(mtxLogger_);
			return loggers_.find(loggerName) != loggers_.end();
		}

		/** 
         * @brief 获取默认日志器
         * @return const Logger::ptr& 默认日志器指针引用
         */
		const Logger::ptr& DefaultLogger() {
			return defaultLogger_;
		}

        /** 
         * @brief 获取 root 日志器
         * @return const Logger::ptr& root 日志器指针引用
         */
        const Logger::ptr& GetRoot() {
            return GetLogger("root");
        }

		/** 
         * @brief 获取指定名称的日志器
         * @param[in] loggerName 日志器名称
         * @return const Logger::ptr& 日志器指针引用（若不存在返回默认日志器）
         */
		const Logger::ptr& GetLogger(const std::string& loggerName) {
			std::unique_lock<std::mutex> ulk(mtxLogger_);
			auto it = loggers_.find(loggerName);
			if (it != loggers_.end()) {
				return it->second;
			}
            std::cerr << "Not found '" << loggerName << "' logger, use default logger instead" << std::endl;
			return defaultLogger_;
		}

        void ReplaceLogger(const Logger::ptr& logger) {
            std::unique_lock<std::mutex> lk(mtxLogger_);
            loggers_[logger->GetLoggerName()] = logger;
        }

        void DelLogger(const std::string& name) {
            std::unique_lock<std::mutex> lk(mtxLogger_);
            if (name == "default") {
                return; // 保护默认 logger（可选）
            }
            
            loggers_.erase(name);
        }

	private:
		LogManager() {
			std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
			builder->BuildName("default");
			defaultLogger_ = builder->Build();
			AddLogger(defaultLogger_);
		}

		LogManager(const LogManager&) = delete;

	private:
		// 用于保证 loggers_(日志器对象集合)线程安全的锁
		std::mutex mtxLogger_;
		// 默认 logger 日志器
		zch::Logger::ptr defaultLogger_;
		// 日志器对象集合
		std::unordered_map<std::string, zch::Logger::ptr> loggers_;
	};
}

#endif
