#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#include "db/Connection.h"
#include "db/ConnectionPool.h"
#include "zchlog.h"

using namespace std;

// 使用连接池进行数据库操作
void op2(ConnectionPool *pool, int begin, int end) {
    for (int i = begin; i < end; ++i) {
        shared_ptr<Connection> conn = pool->GetConnection();
        if (conn) {
            char sql[1024] = {0};
            sprintf(sql, "insert into person values(%d, 25, 'man', 'tom')", i);
            conn->Update(sql);
        } else {
            LOG_ERROR() << "GetConnection failed";
        }
    }
}

void test_pool_single_thread() {
    LOG_INFO() << "Testing ConnectionPool (Single Thread)...";
    ConnectionPool &pool = ConnectionPool::GetConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op2(&pool, 0, 100);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto length = end - begin;
    LOG_INFO() << "连接池, 单线程, 插入100条数据用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒";
}

void test_pool_multi_thread() {
    LOG_INFO() << "Testing ConnectionPool (Multi Thread)...";
    ConnectionPool &pool = ConnectionPool::GetConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    
    thread t1(op2, &pool, 101, 125);
    thread t2(op2, &pool, 126, 150);
    thread t3(op2, &pool, 151, 175);
    thread t4(op2, &pool, 176, 200);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto length = end - begin;
    LOG_INFO() << "连接池, 多线程(4线程), 插入100条数据用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒";
}

int main() {

    // 初始化日志系统 (加载配置文件)
    zch::InitLogFromJson("/home/zch/Project/TinyWebserver/config/log_config.json");

    // 每次运行前清空表，避免主键冲突
    {
        ConnectionPool &pool = ConnectionPool::GetConnectionPool();
        shared_ptr<Connection> conn = pool.GetConnection();
        if (conn) {
            conn->Update("DELETE FROM person");
        }
    }

    test_pool_single_thread();
    LOG_INFO() << "--------------------------------";
    test_pool_multi_thread();
    return 0;
}
