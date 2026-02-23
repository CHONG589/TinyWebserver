// 说明：
// - 封装 MySQL 基本连接与执行能力，提供 Connect / Update / Query 三类接口
// - 维护连接活跃时间，供连接池回收策略参考
// - 采用 RAII 管理连接生命周期，避免资源泄漏

#include "db/Connection.h"

/**
 * @brief 构造函数
 * 初始化 MySQL 连接句柄，并设置字符集为 UTF-8
 */
Connection::Connection() {
    // 初始化连接句柄；若传入非空指针则复用，否则新建
    m_conn = mysql_init(nullptr);
    // 设置字符集编码，确保写入/读取中文等字符不乱码
    mysql_set_character_set(m_conn, "utf8");
}

/**
 * @brief 析构函数
 * 若存在有效连接，自动关闭以释放服务器与客户端资源
 */
Connection::~Connection() {
    if (m_conn != nullptr) {
        // 析构阶段关闭连接，释放服务器端与客户端资源
        mysql_close(m_conn);
    }
}

/**
 * @brief 建立到 MySQL 的连接
 * @param[in] ip   数据库服务器 IP
 * @param[in] port 数据库端口（默认 MySQL 为 3306）
 * @param[in] user 用户名
 * @param[in] pwd  密码
 * @param[in] db   目标数据库名
 * @return     连接成功返回 true，失败返回 false（并打印错误日志）
 *
 * 说明：内部调用 mysql_real_connect 完成握手；调用成功后 _conn 为有效连接句柄
 */
bool Connection::Connect(const std::string &ip, const uint16_t port, const std::string &user, const std::string &pwd,
                         const std::string &db) {
    // 建立到 MySQL 的真实连接；成功则返回非空连接句柄
    m_conn = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), pwd.c_str(), db.c_str(), port, nullptr, 0);
    if (m_conn == nullptr) {
        LOG_ERROR() << "MySQL Connect Error";
        return false;
    }
    return true;
}

/**
 * @brief 执行写操作（INSERT/UPDATE/DELETE 等）
 * @param[in] sql 待执行的 SQL 语句
 * @return    成功返回 true，失败返回 false（并打印错误日志）
 *
 * 说明：内部调用 mysql_query 执行；不返回受影响行数，若需请扩展接口
 */
bool Connection::Update(const std::string &sql) {
    // 执行写语句（INSERT/UPDATE/DELETE 等）；返回 0 表示成功
    if (mysql_query(m_conn, sql.c_str()) != 0) {
        // mysql_error(m_conn) 返回错误字符串，便于定位问题
        LOG_INFO() << "SQL " << sql << " 更新失败：" << mysql_error(m_conn);
        return false;
    }
    return true;
}

/**
 * @brief 执行查询操作（SELECT 等）
 * @param[in] sql 待执行的 SQL 语句
 * @return    返回结果集指针（MYSQL_RES*）；失败返回 nullptr（并打印错误日志）
 *
 * 使用注意：
 * - 返回值由 mysql_use_result 获取，为流式读取；读取完毕务必调用 mysql_free_result 释放
 * - 在读取过程中，连接处于忙状态；未释放结果集会阻塞后续操作
 */
MYSQL_RES *Connection::Query(const std::string &sql) {
    // 执行查询语句；非 0 表示失败
    if (mysql_query(m_conn, sql.c_str()) != 0) {
        // 返回 nullptr 表示失败；请根据业务场景做判空处理
        LOG_INFO() << "SQL " << sql << " 查询失败：" << mysql_error(m_conn);
        return nullptr;
    }
    // 使用流式结果集读取；使用方应在读取完毕后调用 mysql_free_result 释放
    return mysql_use_result(m_conn);
}

/**
 * @brief 刷新连接的存活时间戳为当前时间
 * 调用方用完连接后，通过智能指针的自定义析构把连接
 * 放回队列，同时调用 RefreshAliveTime，这样下一
 * 次空闲计时从“归还时刻”重新开始。
 * 说明：连接池依据该时间计算空闲时长并执行回收策略
 */
void Connection::RefreshAliveTime() {
    // 刷新连接最近活跃时间，用于池内空闲扫描与回收
    // 调用方用完连接后，通过智能指针的自定义析构把连接
    // 放回队列，同时调用 RefreshAliveTime，这样下一
    // 次空闲计时从“归还时刻”重新开始。
    m_aliveTime = std::chrono::steady_clock::now();
}

/**
 * @brief 获取连接自上次刷新以来的存活时长
 * @return 距离上次 RefreshAliveTime 的时间差（单位：微秒）
 */
long long Connection::GetAliveTime() const {
    // 返回自上次刷新以来的时间差（微秒）
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_aliveTime).count();
}
