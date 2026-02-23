#include <iostream>
#include <memory>
#include <chrono>

#include "db/Connection.h"
#include "zchlog.h"

using namespace std;

// 单线程: 不使用连接池进行数据库操作
void op1(int begin, int end) {
    for (int i = begin; i < end; ++i) {
        Connection conn;
        // 注意：这里需要根据实际数据库配置修改参数，或者改进为从配置读取
        if (conn.Connect("127.0.0.1", 3306, "root", "589520", "testdb")) {
            char sql[1024] = {0};
            sprintf(sql, "insert into person values(%d, 25, 'man', 'tom')", i);
            conn.Update(sql);
        } else {
            cerr << "Connect failed" << endl;
        }
    }
}

int main() {
    cout << "Testing Connection (Single Thread, No Pool)..." << endl;

    // 初始化日志系统 (加载配置文件)
    zch::InitLogFromJson("/home/zch/Project/TinyWebserver/config/log_config.json");
    
    // 每次运行前清空表，避免主键冲突
    {
        Connection conn;
        if (conn.Connect("127.0.0.1", 3306, "root", "589520", "testdb")) {
            conn.Update("DELETE FROM person");
        }
    }

    // 单线程测试
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op1(0, 100); // 插入100条数据
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    
    auto length = end - begin;
    cout << "非连接池, 单线程, 插入100条数据用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒" << endl;

    return 0;
}
