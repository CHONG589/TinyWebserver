#include "Formatter.h"
#include "LogSink.h"
#include "Logger.h"
#include "Log.h"
#include "LogConfig.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#endif

int main() {
#ifdef _WIN32
    // 设置控制台编码为 UTF-8，防止中文乱码
    SetConsoleOutputCP(CP_UTF8);
#endif

	zch::InitLogFromJson("/home/zch/Project/zchlog/log_config.json");
	zch::Logger::ptr logger = zch::LogManager::GetInstance().GetLogger("zchlog");

	LOG_DEBUG_TO(logger) << "jintian";
	int i = 0;
	LOG_DEBUG_TO(logger) << i;

	LOG_INFO_TO(logger) << "zengchong";
	LOG_INFO_BY_NAME("zchlog") << "LINGMEI";

    // 测试默认 root logger
    LOG_INFO() << "This is a log from default root logger";

    return 0;
}
