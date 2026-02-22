// 说明：
// 本类封装了对 MySQL C API 的最小连接与操作能力，负责：
// - 连接管理：连接建立（Connect）、析构时自动关闭（RAII）
// - 数据写操作：执行增删改语句（Update）
// - 数据读操作：执行查询并返回结果集指针（Query）
// - 存活时间：配合连接池计算连接空闲时长（RefreshAliveTime / GetAliveTime）
//
// 设计要点：
// - 线程安全：单个 Connection 实例不保证跨线程安全；每个线程应独立获取并使用连接
// - 生命周期：构造时初始化句柄，析构时关闭连接，避免资源泄漏
// - 编码设置：构造函数中设置连接编码为 UTF-8，确保中文等字符正确处理
// - 结果集管理：Query 返回 MYSQL_RES*，使用方需在读取完数据后调用 mysql_free_result 释放
// - 与连接池协作：aliveTime 用于池内回收策略（超时空闲连接）
//

#ifndef CLUSTERCHATSERVER_CONNECTION_H
#define CLUSTERCHATSERVER_CONNECTION_H

#include <mysql/mysql.h>
#include <chrono>
#include <string>
#include "zchlog.h"

class Connection {
public:
    /**
     * 构造函数
     * 初始化 MySQL 连接句柄，并设置字符集为 UTF-8
     */
    Connection();

    /**
     * 析构函数
     * 若存在有效连接，自动关闭以释放服务器与客户端资源
     */
    ~Connection();

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
    bool Connect(const std::string &ip, const uint16_t port, const std::string &user, const std::string &pwd,
                 const std::string &db);

    /**
     * @brief 执行写操作（INSERT/UPDATE/DELETE 等）
     * @param[in] sql 待执行的 SQL 语句
     * @return    成功返回 true，失败返回 false（并打印错误日志）
     *
     * 说明：内部调用 mysql_query 执行；不返回受影响行数，若需请扩展接口
     */
    bool Update(const std::string &sql);

    /**
     * @brief 执行查询操作（SELECT 等）
     * @param[in] sql 待执行的 SQL 语句
     * @return    返回结果集指针（MYSQL_RES*）；失败返回 nullptr（并打印错误日志）
     *
     * 使用注意：
     * - 返回值由 mysql_use_result 获取，为流式读取；读取完毕务必调用 mysql_free_result 释放
     * - 在读取过程中，连接处于忙状态；未释放结果集会阻塞后续操作
     */
    MYSQL_RES *Query(const std::string &sql);

    /**
     * @brief 刷新连接的存活时间戳为当前时间
     * 调用方用完连接后，通过智能指针的自定义析构把连接
     * 放回队列，同时调用 RefreshAliveTime，这样下一
     * 次空闲计时从“归还时刻”重新开始。
     * 说明：连接池依据该时间计算空闲时长并执行回收策略
     */
    void RefreshAliveTime();

    /**
     * @brief 获取连接自上次刷新以来的存活时长
     * @return 距离上次 RefreshAliveTime 的时间差（单位：微秒）
     */
    long long GetAliveTime() const;

private:
    // MySQL C API 的连接句柄；通过 mysql_init / mysql_real_connect 获得
    MYSQL *_conn;
    // 最近一次活跃时间；用于连接池空闲连接的扫描与回收
    // 用来衡量这条连接在连接池里已经闲置了多久
    std::chrono::time_point<std::chrono::steady_clock> _aliveTime;
};

#endif //CLUSTERCHATSERVER_CONNECTION_H
