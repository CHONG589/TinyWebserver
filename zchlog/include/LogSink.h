/**
 * @file LogSink.h
 * @brief 实现日志的输出方式，并支持输出方式的扩展
 *          - 标准输出
 *          - 指定文件
 *          - 滚动文件
 * @author zch
 * @date 2025-11-02
 */

#ifndef LOGSINK_H__
#define LOGSINK_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
//#include <json/json.h>

#include "util.hpp"
//#include "../include/MySQLConn.h"

namespace zch {

    // 日志输出方式基类
	class LogSink {
	public:
		using ptr = std::shared_ptr<LogSink>;

        /** 
         * @brief 日志输出纯虚接口
         * @param[in] data 日志数据地址
         * @param[in] len 日志数据长度
         * @return void
         */
		virtual void log(const char* data, size_t len) = 0;
		virtual ~LogSink() {};
	};

    // 标准输出
	class StdOutSink : public LogSink {
	public:
        /** 
         * @brief 输出到标准输出
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
		void log(const char* data, size_t len) override {
			std::cout.write(data, len);
            // 强制刷新缓冲区，确保立即输出
            std::cout.flush();
		}
	};

    // 指定文件
	class FileSink : public LogSink {
	public:
		// 创建文件并打开
        /** 
         * @brief 构造函数，打开指定日志文件
         * @param[in] pathname 文件路径
         */
		FileSink(const std::string& pathname);

        /** 
         * @brief 输出到文件
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
		void log(const char* data, size_t len) override {
			ofs_.write(data, len);
		}

	private:
		std::string pathName_;
		std::ofstream ofs_;
	};

    // 滚动文件(这里按照文件大小进行滚动)
	class RollBySizeSink : public LogSink {
	public:
        /** 
         * @brief 构造函数（默认当前目录）
         * @param[in] maxSize 单个日志文件最大大小（字节）
         */
		RollBySizeSink(size_t maxSize);

        /** 
         * @brief 构造函数（指定目录）
         * @param[in] baseDir 日志存放目录
         * @param[in] maxSize 单个日志文件最大大小（字节）
         */
		RollBySizeSink(const std::string& baseDir, size_t maxSize);

        /** 
         * @brief 输出到滚动文件
         * @param[in] data 日志数据
         * @param[in] len 数据长度
         * @return void
         */
		void log(const char* data, size_t len) override;

	private:
		// 得到要生成的日志文件的名称
		// 通过基础文件名 + 时间组成 + 计数器生成真正的文件名
        /** 
         * @brief 生成滚动日志文件名
         * @return std::string 包含时间戳和序号的文件名
         */
		std::string GetFileName();

	private:
        // 基础文件名
		std::string baseName_;
		// 文件大小限制
		size_t maxSize_;
		// 当前文件的大小
		size_t curSize_;
		std::ofstream ofs_;
		// 文件名称计数器(防止一秒之内创建多个文件时，使用同一个名称)
		size_t nameCout_;
	};

    // 滚动间隔类型
    enum class TimeGap {
        GAP_DAY,
        GAP_HOUR
    };

    // 按时间滚动文件
    class RollByTimeSink : public LogSink {
    public:
        /** 
         * @brief 构造函数
         * @param[in] baseDir 日志输出目录
         * @param[in] gap_type 滚动间隔（天/小时）
         */
        RollByTimeSink(const std::string& baseDir, TimeGap gap_type);

        /** 
         * @brief 输出日志
         * @param[in] data 数据
         * @param[in] len 长度
         */
        void log(const char* data, size_t len) override;

    private:
        /** 
         * @brief 根据时间生成文件名
         * @param[in] t 时间结构体
         * @return std::string 文件名
         */
        std::string GetFileName(const struct tm& t);

        /** 
         * @brief 创建新文件并更新时间步长
         * @return void
         */
        void CreateNewFile();

    private:
        std::string baseDir_;
        TimeGap gapType_;
        std::ofstream ofs_;
        // 当前日志文件对应的时间步长（用于判断是否需要滚动）
        time_t curStep_; 
    };

    // // MySQL 服务器中
	// class MySQLSink : public LogSink {
	// public:
	// 	MySQLSink();
	// 	void log(const char* data, size_t len) override;

	// private:
	// 	MySQLConn _conn;
	// };

    // 落地方向类的工厂类  (通过此工厂类实现对落地方向的可扩展性)
	class SinkFactory {
	public:
        /** 
         * @brief 创建日志 Sink 对象
         * @tparam SinkType Sink 类型
         * @tparam Args 构造参数类型包
         * @param[in] args 构造参数
         * @return LogSink::ptr Sink 对象智能指针
         */
		template<class SinkType, class ...Args>
		static LogSink::ptr create(Args&&... args) {
			return std::make_shared<SinkType>(std::forward<Args>(args)...);
		}
	};
}

#endif
