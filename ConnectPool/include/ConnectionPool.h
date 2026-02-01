//
// Created by Cmf on 2022/8/24.
//
// 说明：
// 连接池用于复用 MySQL 连接，降低频繁创建/销毁连接的开销，并实现多线程下的高效数据库访问。
// 主要能力：
// - 单例访问：提供唯一的 ConnectionPool 实例（线程安全的静态局部初始化）
// - 获取连接：阻塞等待空闲连接，返回带自定义析构的智能指针，析构时归还队列
// - 生产连接：独立线程在连接不足时按需创建新连接（不超过最大值）
// - 回收连接：独立线程定期扫描并回收超过最大空闲时间的连接，保持资源占用可控
// - 配置加载：从 JSON 配置文件读取数据库与池参数
//
// 线程安全：
// - 使用 std::mutex + std::condition_variable 保护连接队列与协调生产/消费
// - _connectionCount 记录当前创建的连接总数，配合队列与扫描维护池规模
//

#ifndef CLUSTERCHATSERVER_COMMOMCONNECTIONPOOL_H
#define CLUSTERCHATSERVER_COMMOMCONNECTIONPOOL_H

#include "noncopyable.hpp"
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "Connection.h"

class ConnectionPool : private noncopyable {
public:
    /**
     * 获取连接池单例
     * 说明：C++11 之后静态局部变量初始化是线程安全的
     */
    static ConnectionPool& GetConnectionPool();
    /**
     * 获取一个可用连接（阻塞式）
     * - 当队列为空时，等待 _connectionTimeout 时间；若超时仍为空则继续等待（重试）
     * - 返回值为 shared_ptr，析构时通过自定义删除器归还连接到队列，并刷新活跃时间
     */
    std::shared_ptr<Connection> GetConnection();

    ~ConnectionPool();

private:
    /**
     * 构造函数
     * - 加载配置，初始化最小数量的连接
     * - 启动生产者线程与扫描回收线程（均为守护线程）
     */
    ConnectionPool();

    /**
     * 加载 JSON 配置文件
     * @return 成功返回 true，失败返回 false
     * 说明：校验每个字段的类型有效性，并设置池参数与数据库连接信息
     */
    bool LoadConfigFile();

    /**
     * 连接生产者任务（独立线程）
     * - 当队列大小小于 _minSize 且总连接数未达到 _maxSize 时创建新连接
     * - 与消费者通过条件变量协作，避免忙等
     */
    void ProduceConnectionTask();

    /**
     * 空闲连接扫描回收任务（独立线程）
     * - 每隔 _maxIdleTime 秒检查队头连接的闲置时长，超过阈值则回收
     * - 队列按归还时间近似从旧到新；队头不超阈值时后续更“新”的连接也不会超
     * - 注意：GetAliveTime 返回微秒，这里比较使用 _maxIdleTime * 1000 的单位（毫秒），
     *         如需严格一致可统一为同一单位（比如全部按微秒）
     */
    void ScannerConnectionTask();

    /**
     * 创建并入队一个新连接
     * - 连接成功后刷新活跃时间并推入队列，增加 _connectionCount
     */
    void AddConnection();

private:
    // 数据库连接信息
    std::string _ip;
    uint16_t _port;
    std::string _user;
    std::string _pwd;
    std::string _db;

    // 连接池参数
    size_t _minSize;             // 最小连接数量（保持的基础容量，生产者维持不低于该值）
    size_t _maxSize;             // 最大连接数量（连接总数上限）
    size_t _maxIdleTime;         // 最大空闲时间（秒，扫描线程的休眠周期与阈值依据）
    size_t _connectionTimeout;   // 获取连接的阻塞等待时长（微秒）

    // 存储连接的队列（原始指针）；归还时入队，回收/析构时逐一删除
    std::queue<Connection *> _connectionQueue;
    // 连接队列的并发保护
    std::mutex _mtx;
    // 当前创建的连接总数（随创建/回收变化）
    std::atomic_int _connectionCount;
    // 条件变量：用于连接生产与消费线程的通信协调
    std::condition_variable _cv;
};

#endif //CLUSTERCHATSERVER_COMMOMCONNECTIONPOOL_H
