//
// Created by Cmf on 2022/8/24.
//
// 文件说明：
// - 封装 MySQL 基本连接与执行能力，提供 Connect / Update / Query 三类接口
// - 维护连接活跃时间，供连接池回收策略参考
// - 采用 RAII 管理连接生命周期，避免资源泄漏

#include "Connection.h"

Connection::Connection() {
    // 初始化连接句柄；若传入非空指针则复用，否则新建
    _conn = mysql_init(nullptr);
    // 设置字符集编码，确保写入/读取中文等字符不乱码
    mysql_set_character_set(_conn, "utf8");
}

Connection::~Connection() {
    if (_conn != nullptr) {
        // 析构阶段关闭连接，释放服务器端与客户端资源
        mysql_close(_conn);
    }
}

bool Connection::Connect(const std::string &ip, const uint16_t port, const std::string &user, const std::string &pwd,
                         const std::string &db) {
    // 建立到 MySQL 的真实连接；成功则返回非空连接句柄
    _conn = mysql_real_connect(_conn, ip.c_str(), user.c_str(), pwd.c_str(), db.c_str(), port, nullptr, 0);
    if (_conn == nullptr) {
        LOG_ERROR("MySQL Connect Error")
        return false;
    }
    return true;
}

bool Connection::Update(const std::string &sql) {
    // 执行写语句（INSERT/UPDATE/DELETE 等）；返回 0 表示成功
    if (mysql_query(_conn, sql.c_str()) != 0) {
        // mysql_error(_conn) 返回错误字符串，便于定位问题
        LOG_INFO("SQL %s 更新失败：%d", sql.c_str(), mysql_error(_conn));
        return false;
    }
    return true;
}

MYSQL_RES *Connection::Query(const std::string &sql) {
    // 执行查询语句；非 0 表示失败
    if (mysql_query(_conn, sql.c_str()) != 0) {
        // 返回 nullptr 表示失败；请根据业务场景做判空处理
        LOG_INFO("SQL %s 查询失败：%d", sql.c_str(), mysql_error(_conn));
        return nullptr;
    }
    // 使用流式结果集读取；使用方应在读取完毕后调用 mysql_free_result 释放
    return mysql_use_result(_conn);
}

void Connection::RefreshAliveTime() {
    // 刷新连接最近活跃时间，用于池内空闲扫描与回收
    // 调用方用完连接后，通过智能指针的自定义析构把连接
    // 放回队列，同时调用 RefreshAliveTime，这样下一
    // 次空闲计时从“归还时刻”重新开始。
    _aliveTime = std::chrono::steady_clock::now();
}

long long Connection::GetAliveTime() const {
    // 返回自上次刷新以来的时间差（微秒）
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _aliveTime).count();
}
