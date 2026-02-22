#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "LogSink.h"
#include "util.hpp"

// 测试 RollByTimeSink 是否能正常创建文件
TEST(RollByTimeTest, CreateFile) {
    std::string test_dir = "test_logs_time";
    // 如果存在先清理
    if (zch::File::IsExist(test_dir)) {
        // 这里简单起见，不递归删除了，假设它是干净的或者空的
    }
    
    // 按天滚动
    {
        zch::RollByTimeSink sink(test_dir, zch::TimeGap::GAP_DAY);
        const char* msg = "hello time rolling";
        sink.log(msg, strlen(msg));
    }
    
    // 验证文件是否生成
    // 文件名应该是 app-YYYY-MM-DD.log
    time_t now = zch::Date::Now();
    struct tm t;
    zch::Date::LocalTime(&now, &t);
    
    std::stringstream ssm;
    ssm << test_dir << "/" << t.tm_year + 1900 << "-" << t.tm_mon + 1 << "-" << t.tm_mday << ".log";
    std::string expected_file = ssm.str();
    
    EXPECT_TRUE(zch::File::IsExist(expected_file));
}
