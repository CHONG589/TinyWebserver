/**
 * @file Formatter.h
 * @brief 日志输出格式化类
 * @author zch
 * @date 2025-11-02
 */

#ifndef FORMATTER_H__
#define FORMATTER_H__

#include <iostream>
#include <sstream>
#include <memory>
#include <ctime>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "LogMsg.h"

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

namespace zch {

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
		virtual void Format(std::ostream& out, const LogMsg& msg) = 0;
	};

    // 日期格式化子项
	class TimeFormatItem : public FormatItem {
	public:
        /** 
         * @brief 构造函数
         * @param[in] fmt 时间格式化字符串（如 %Y-%m-%d %H:%M:%S）
         */
		TimeFormatItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S") : timeFmt_(fmt) {}

        /** 
         * @brief 格式化时间戳
         * @param[in,out] oss 输出流
         * @param[in] msg 日志消息
         * @return void
         */
		void Format(std::ostream& oss, const LogMsg& msg) override {
			struct tm t;
			char buf[32] = { 0 };
			// 使用C库函数对时间戳进行格式化
			Date::LocalTime(&msg.ctime_, &t);
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
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中
			oss << LogLevel::ToString(msg.level_);
		}
	};

    // 日志器名称格式化子项
	class LoggerFormatItem : public FormatItem {
	public:
        /** 
         * @brief 格式化日志器名称
         * @param[in,out] oss 输出流
         * @param[in] msg 日志消息
         * @return void
         */
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中
			oss << msg.logger_;
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
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中
			oss << msg.tid_;
		}
	};

    // 文件名称格式化子项
	class FileFormatItem : public FormatItem {
	public:
        /** 
         * @brief 格式化文件名
         * @param[in,out] oss 输出流
         * @param[in] msg 日志消息
         * @return void
         */
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中
			oss << msg.file_;
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
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中
			oss << msg.line_;
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
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中，输出消息前面加了一个空格
			oss << " " << msg.payload_;
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
		void Format(std::ostream& oss, const LogMsg& msg) override {
			// 提取指定字段插入流中
			oss << "\t";
		}
	};

    // 新行格式化子项
	class NLineFormatItem : public FormatItem {
	public:
        /** 
         * @brief 输出换行符
         * @param[in,out] oss 输出流
         * @param[in] msg 日志消息
         * @return void
         */
		void Format(std::ostream& oss, const LogMsg& msg) override {
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
		void Format(std::ostream& oss, const LogMsg& msg) override {
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
		Formatter(const std::string& pattern = "[%d{%Y-%m-%d %H:%M:%S}][%p][%f:%l]%m%n") : pattern_(pattern) {
			assert(ParsePattern());
		}

        /** 
         * @brief 格式化消息并写入流
         * @param[in,out] oss 输出流
         * @param[in] msg 日志消息
         * @return void
         */
		void Format(std::ostream& oss, const LogMsg& msg);

		// 将日志以返回值的形式进行返回
        /** 
         * @brief 格式化消息并返回字符串
         * @param[in] msg 日志消息
         * @return std::string 格式化后的日志字符串
         */
		std::string Format(const LogMsg& msg) {
			// 复用 Format(std::ostream& oss, const LogMsg& msg) 接口
			std::stringstream oss;
			Format(oss, msg);
			return oss.str();
		}

	private:
		// 解析格式化字符串并填充 items 数组
        /** 
         * @brief 解析格式化模式字符串
         * @return bool 解析成功返回 true
         */
		bool ParsePattern();

		// 根据不同的格式化字符创建不同的格式化子类，是 ParsePattern 的子函数
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
}

#endif
