#include <cassert>

#include "LogSink.h"

/** 
 * @brief 构造函数，打开指定日志文件
 * @param[in] pathName 文件路径
 */
zch::FileSink::FileSink(const std::string& pathName)
	                    : pathName_(pathName) {
	// 1.检查路径是否存在,不存在就创建
	if (!zch::File::IsExist(zch::File::GetDirPath(pathName_))) {
		zch::File::CreateDirectory(zch::File::GetDirPath(pathName_));
	}

	// 2. 创建并打开文件
	ofs_.open(pathName_, std::ios::binary | std::ios::app);
	if (!ofs_.is_open()) {
		std::cerr << "FileSink中文件打开失败" << std::endl;
		abort();
	}
}

/** 
 * @brief 构造函数（默认当前目录）
 * @param[in] maxSize 单个日志文件最大大小（字节）
 */
zch::RollBySizeSink::RollBySizeSink(size_t maxSize)
                                    : maxSize_(maxSize)
                                    , curSize_(0)
                                    , nameCout_(0) {
	// 1.检查路径是否存在,不存在就创建
	// if (!zch::File::IsExist(zch::File::GetDirPath(baseName_))) {
	// 	zch::File::CreateDirectory(zch::File::GetDirPath(baseName_));
	// }

	// 2. 获得文件名
	std::string fileName = GetFileName();

	// 3. 创建并打开文件
	ofs_.open(fileName, std::ios::binary | std::ios::app);
	assert(ofs_.is_open());
}

/** 
 * @brief 构造函数（指定目录）
 * @param[in] baseDir 日志存放目录
 * @param[in] maxSize 单个日志文件最大大小（字节）
 */
zch::RollBySizeSink::RollBySizeSink(const std::string& baseDir, size_t maxSize)
    : baseName_(baseDir)
    , maxSize_(maxSize)
    , curSize_(0)
    , nameCout_(0) {
    if (!baseName_.empty()) {
        if (!zch::File::IsExist(baseName_)) {
            zch::File::CreateDirectory(baseName_);
        }
    }
    std::string fileName = GetFileName();
    ofs_.open(fileName, std::ios::binary | std::ios::app);
    assert(ofs_.is_open());
}

/** 
 * @brief 输出到滚动文件
 * @param[in] data 日志数据
 * @param[in] len 数据长度
 * @return void
 */
void zch::RollBySizeSink::log(const char* data, size_t len) {
	// 判断文件是否超出大小
	if (curSize_ >= maxSize_) {
		// 关闭旧文件
		ofs_.close();

		std::string fileName = GetFileName();
		ofs_.open(fileName, std::ios::binary | std::ios::app);
		assert(ofs_.is_open());
		// 由于是新文件，所以将当前文件已写入的大小置 0
		curSize_ = 0;
	}
	ofs_.write(data, len);
    ofs_.flush();
	curSize_ += len;
}

/** 
 * @brief 生成滚动日志文件名
 * @return std::string 包含时间戳和序号的文件名
 */
std::string zch::RollBySizeSink::GetFileName() {
	std::stringstream ssm;
	struct tm t = Date::GetTimeSet();
	if (!baseName_.empty()) {
		ssm << baseName_;
		if (baseName_.back() != '/' && baseName_.back() != '\\') {
			ssm << '/';
		}
	}
	ssm << t.tm_year + 1900;
	ssm << '-';
	ssm << t.tm_mon + 1;
	ssm << '-';
	ssm << t.tm_mday;
	ssm << '-';
	ssm << std::to_string(nameCout_++);
	ssm << ".log";
	return ssm.str();
}

// --- RollByTimeSink Implementation ---

zch::RollByTimeSink::RollByTimeSink(const std::string& baseDir, TimeGap gap_type)
    : baseDir_(baseDir), gapType_(gap_type) {
    // 1. 创建目录
    if (!baseDir_.empty()) {
        if (!zch::File::IsExist(baseDir_)) {
            zch::File::CreateDirectory(baseDir_);
        }
    }
    // 2. 初始化文件
    CreateNewFile();
}

void zch::RollByTimeSink::log(const char* data, size_t len) {
    time_t cur = Date::Now();
    // 计算当前时间的步长
    // 这里要注意：直接除以秒数得到的是基于 Epoch 的步长，这在跨时区时可能会有偏移，
    // 但对于判断“是否变了”是足够的。为了更精确地对齐自然天，可以用 localtime。
    // 为了性能，我们先简单除法。
    time_t cur_gap = (gapType_ == TimeGap::GAP_DAY) ? (cur / (24 * 60 * 60)) : (cur / (60 * 60));

    if (cur_gap != curStep_) {
        // 关闭旧文件，开启新文件
        CreateNewFile();
    }
    ofs_.write(data, len);
    ofs_.flush();
}

void zch::RollByTimeSink::CreateNewFile() {
    if (ofs_.is_open()) {
        ofs_.close();
    }
    // 获取当前时间用于生成文件名
    time_t t_now = Date::Now();
    struct tm t;
    Date::LocalTime(&t_now, &t);
    
    // 更新当前步长
    curStep_ = (gapType_ == TimeGap::GAP_DAY) ? (t_now / (24 * 60 * 60)) : (t_now / (60 * 60));
    
    std::string filename = GetFileName(t);
    ofs_.open(filename, std::ios::binary | std::ios::app);
    assert(ofs_.is_open());
}

std::string zch::RollByTimeSink::GetFileName(const struct tm& t) {
    std::stringstream ssm;
    if (!baseDir_.empty()) {
        ssm << baseDir_;
        if (baseDir_.back() != '/' && baseDir_.back() != '\\') {
            ssm << '/';
        }
    }

    ssm << t.tm_year + 1900 << "-" << t.tm_mon + 1 << "-" << t.tm_mday;
    if (gapType_ == TimeGap::GAP_HOUR) {
        ssm << "-" << t.tm_hour;
    }
    ssm << ".log";
    return ssm.str();
}
