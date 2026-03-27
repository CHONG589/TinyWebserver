/**
 * @file log.cpp
 * @brief 日志模块
 * @date 2026-03-27
 */

#include "log.h"

namespace zch {

const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name) case LogLevel::name: return #name;
    XX(FATAL);
    XX(ALERT);
    XX(CRIT);
    XX(ERROR);
    XX(WARN);
    XX(NOTICE);
    XX(INFO);
    XX(DEBUG);
#undef XX
    default:
        return "NOTSET";
    }
    return "NOTSET";
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) if(str == #v) { return LogLevel::level; }
    XX(FATAL, fatal);
    XX(ALERT, alert);
    XX(CRIT, crit);
    XX(ERROR, error);
    XX(WARN, warn);
    XX(NOTICE, notice);
    XX(INFO, info);
    XX(DEBUG, debug);

    XX(FATAL, FATAL);
    XX(ALERT, ALERT);
    XX(CRIT, CRIT);
    XX(ERROR, ERROR);
    XX(WARN, WARN);
    XX(NOTICE, NOTICE);
    XX(INFO, INFO);
    XX(DEBUG, DEBUG);
#undef XX

    return LogLevel::NOTSET;
}

/** 
 * @brief 格式化消息并写入流
 * @param[in,out] oss 输出流
 * @param[in] msg 日志消息
 * @return void
 */
void zch::Formatter::Format(std::ostream& oss, LogMsg::ptr msg) {
	// 不断循环取出每一个对象
	for (auto& it : items_) {
		// 对每一个格式化子项调用自己的 Format 接口，格式化指定部分然后输出到流当中
		// 当所有的格式化子项都被调用完毕以后就组成了一条完整的日志了。
		it->Format(oss, msg);
	}
}

/** 
 * @brief 解析格式化模式字符串
 * @return bool 解析成功返回 true
 */
bool zch::Formatter::ParsePattern() {
    // 从前往后遍历，如果没有遇到%则说明之前的字符都是原始字符串；
    // 遇到%，则看紧随其后的是不是另一个%，如果是，则认为是在转义字符串，%%等于一个%；
    // 如果%后面紧挨着的是格式化字符（c、f、l、t等），则进行处理；
    // 紧随格式化字符之后，如果有{，则认为在{之后、}之前都是子格式内容。

	// 例如 abcd%%e[%d{%H:%M:%S}][%p][%c][%t]%T%m%n
	// 可以看到上面的示例中 `[` 前面的字符都是非格式化字符

    // 在处理过程中，我们需要将处理的结果保存下来，于是我们可以创建一个一个键值对(key， val)。
    // 如果是格式化字符，则 key 为该格式化字符，val 为 null 或者是子格式化字符串；若为原始字符串
    // 则 key 为 null，val 为原始字符串内容

	// 一、解析格式化字符串
	// 有效的格式化字符集合
	std::unordered_set<char> fmt_set = { 'd','p','c','t','f','l','m','T','n' };

	// 存储格式化字符的顺序
	// 其中 pair 的第一个参数是：格式化字符，第二个参数是：创建格式化子项时对应的参数
	std::vector<std::pair<std::string, std::string>> fmt_order;

	std::string key, val;
	// 字符串的处理位置
	size_t pos = 0;
	// 当 pos 越界时,表示 pattern 解析完毕
	while (pos < pattern_.size()) {
		// 1. 先处理原始字符串，格式化字符都是以%开头的
		if (pattern_[pos] != '%') {
			// 原始字符串只有 val，没有 key
			val.push_back(pattern_[pos++]);
			continue;
		}

		// 1.2 对%%进行处理转义,(走到这里当前位置一定等于%)
		if (pos + 1 < pattern_.size() && pattern_[pos + 1] == '%') {
			val.push_back('%');
			pos += 2;
			continue;
		}

		// 1.3 将组织好原始字符串放入 fmt_order 中
		if (!val.empty()) {
			fmt_order.push_back(std::make_pair("", val));
			val.clear();
		}

		// 2 处理格式化字符
		// 2.1 走到这里说明当前位置是%,下一个字符可能是格式化字符
		if (pos + 1 >= pattern_.size()) {
			std::cerr << "% 后无格式化字符！" << std::endl;
			return false;
		} else {
			auto it = fmt_set.find(pattern_[pos + 1]);
			if (it != fmt_set.end()) {
				key.push_back(pattern_[pos + 1]);
				pos += 2;
			} else {
				std::cerr << "格式化字符不正确" << std::endl;
				return false;
			}
		}

		// 3.处理%d的格式化子串 如{%H:%M:%S}
		if (pos < pattern_.size() && pattern_[pos] == '{') {
			size_t end_pos = pattern_.find_first_of('}', pos + 1);
			if (end_pos == std::string::npos) {
				std::cerr << "格式化子串{}匹配出现问题" << std::endl;
				return false;
			} else {
				val += pattern_.substr(pos + 1, end_pos - pos - 1);
				pos = end_pos + 1;
			}
		}
		fmt_order.push_back(std::make_pair(key, val));
		key.clear();
		val.clear();
	}

	// 二、使用解析完毕的数据初始化格式化子项数组
	for (auto& it : fmt_order) {
		items_.push_back(CreateItem(it.first, it.second));
	}
	return true;
}

/**
 * @brief 创建格式化子项
 * @param[in] key 格式化字符
 * @param[in] val 构造每一个格式化子项时要传递的参数
 * @return FormatItem::ptr 格式化子项指针
 */
inline zch::FormatItem::ptr zch::Formatter::CreateItem(const std::string& key, const std::string& val) {
	// 构造日期格式化子项
	if (key == "d") {
        // 构造「时间格式化子项」时，我们要传递 %H:%M:%S 这样的参数
        return std::make_shared<TimeFormatItem>(val);
    }
	// 构造日志级别格式化子项
	if (key == "p") {
        return std::make_shared<LevelFormatItem>();
    }
	// 构造日志器名称格式化子项
	if (key == "c") {
        return std::make_shared<LoggerNameFormatItem>();
    }
	// 构造线程id格式化子项
	if (key == "t") {
        return std::make_shared<ThreadFormatItem>();
    }
	// 构造文件名格式化子项
	if (key == "f") {
        return std::make_shared<FileNameFormatItem>();
    }
	// 构造行号格式化子项
	if (key == "l") {
        return std::make_shared<LineFormatItem>();
    }
	// 构造日志消息格式化子项
	if (key == "m") {
        return std::make_shared<MsgFormatItem>();
    }
	// 构造缩进格式化子项
	if (key == "T") {
        return std::make_shared<TabFormatItem>();
    }
	// 构造换行格式化子项
	if (key == "n") {
        return std::make_shared<NewLineFormatItem>();
    }
	// 没有匹配的格式化字符就构造 其他格式化子项
	return std::make_shared<OtherFormatItem>(val);
}

LogAppender::LogAppender(Formatter::ptr default_formatter)
    : m_defaultFormatter(default_formatter) {
}

void LogAppender::SetFormatter(Formatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
}

Formatter::ptr LogAppender::GetFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter ? m_formatter : m_defaultFormatter;
}

StdoutLogAppender::StdoutLogAppender()
    : LogAppender(Formatter::ptr(new Formatter)) {
}

void StdoutLogAppender::Log(LogMsg::ptr event) {
    MutexType::Lock lock(m_mutex);
    if(m_formatter) {
        m_formatter->Format(std::cout, event);
    } else {
        m_defaultFormatter->Format(std::cout, event);
    }
}

std::string StdoutLogAppender::ToYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["pattern"] = m_formatter->GetPattern();
    std::stringstream ss;
    ss << node;

    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string &file)
    : LogAppender(Formatter::ptr(new Formatter)) {
    m_filename = file;
    ReOpen();
    if(m_reopenError) {
        std::cout << "reopen file " << m_filename << " error" << std::endl;
    }
}

/**
 * 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
 */
void FileLogAppender::Log(LogMsg::ptr event) {
    uint64_t now = event->GetTime();
    if(now >= (m_lastTime + 3)) {
        //reopen();
        if(m_reopenError) {
            std::cout << "reopen file " << m_filename << " error" << std::endl;
        }
        m_lastTime = now;
    }
    if(m_reopenError) {
        return;
    }
    MutexType::Lock lock(m_mutex);
    if(m_formatter) {
        // if(!m_formatter->format(m_filestream, event)) {
        //     std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        // }
    } else {
        // if(!m_defaultFormatter->format(m_filestream, event)) {
        //     std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        // }
    }
    
}

}
