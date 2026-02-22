#include <gtest/gtest.h>
#include "Formatter.h"

TEST(FormatterTest, BasicFormat) {
    // 格式: [等级] 消息
    // 注意: MsgFormatItem 内部会自动加一个空格
    zch::Formatter fmt("[%p]%m");
    zch::LogMsg msg(zch::LogLevel::Level::INFO, "testLogger", "testFile.cpp", 10, "hello");
    
    std::string result = fmt.Format(msg);
    EXPECT_EQ(result, "[INFO] hello");
}

TEST(FormatterTest, FullFormat) {
    // 格式: [%p][%c]%m
    zch::Formatter fmt("[%p][%c]%m");
    zch::LogMsg msg(zch::LogLevel::Level::DEBUG, "App", "main.cpp", 100, "debug message");
    
    std::string result = fmt.Format(msg);
    EXPECT_EQ(result, "[DEBUG][App] debug message");
}

TEST(FormatterTest, PatternParsing) {
    // 测试解析包含各种字符的模式
    EXPECT_NO_THROW({
        zch::Formatter fmt("%d{%H:%M:%S} %T [%t] %p %c %f:%l %m %n");
    });
}
