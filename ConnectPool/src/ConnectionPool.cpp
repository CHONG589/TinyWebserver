//
// Created by Cmf on 2022/8/24.
//
// 文件说明：
// - 提供连接池的单例获取、连接获取（带自定义析构归还）、连接生产与空闲回收
// - 通过 JSON 文件加载数据库与池参数，初始化最小连接数，并启动后台守护线程
// - 关键并发原语：std::mutex + std::condition_variable 保护队列与线程协作
#include <fstream>
#include "ConnectionPool.h"
#include "json.hpp"

using json = nlohmann::json;

ConnectionPool &ConnectionPool::GetConnectionPool() {
    // C++11 线程安全的静态局部变量初始化，保证单例获取在并发场景下安全
    static ConnectionPool pool;
    return pool;
}

std::shared_ptr<Connection> ConnectionPool::GetConnection() {
    // 消费者：获取连接
    // - 当队列为空时，等待 _connectionTimeout（微秒）；若超时仍为空则继续等待（重试）
    // - 返回 shared_ptr<Connection>，自定义删除器在智能指针析构时归还连接并刷新活跃时间
    std::unique_lock<std::mutex> lock(_mtx);
    while (_connectionQueue.empty()) {  //连接为空，就阻塞等待_connectionTimeout时间，如果时间过了，还没唤醒
        if (std::cv_status::timeout == _cv.wait_for(lock, std::chrono::microseconds(_connectionTimeout))) {
            if (_connectionQueue.empty()) { //就可能还是为空
                continue;
            }
        }
    }
    // 对于使用完成的连接，不能直接销毁该连接，而是需要将该连接归还给连接池的队列，供之后的其他消费者使用，
    // 于是我们使用智能指针，自定义其析构函数，完成放回的操作：
    std::shared_ptr<Connection> res(_connectionQueue.front(), [&](Connection *conn) {
        std::unique_lock<std::mutex> locker(_mtx);
        // 归还前刷新活跃时间，使得该连接在池中的空闲计时从“当前时刻”重新开始
        conn->RefreshAliveTime();
        _connectionQueue.push(conn);
    });
    _connectionQueue.pop();
    _cv.notify_all();
    return res;
}

ConnectionPool::ConnectionPool() {
    // 加载配置失败则不继续初始化
    if (!LoadConfigFile()) {
        LOG_ERROR("JSON Config Error");
        return;
    }
    // 创建初始数量的连接（维持不低于 _minSize）
    for (int i = 0; i < _minSize; ++i) {
        AddConnection();
    }
    // 启动一个新的线程，作为连接的生产者 linux thread => pthread_create
    std::thread produce(std::bind(&ConnectionPool::ProduceConnectionTask, this));
    // 守护线程，与主线程分离
    produce.detach();
    // 启动一个新的定时线程，扫描超过 maxIdleTime 时间的空闲连接，进行对于的连接回收
    std::thread scanner(std::bind(&ConnectionPool::ScannerConnectionTask, this));
    // 守护线程，与主线程分离
    scanner.detach();
}

ConnectionPool::~ConnectionPool() {
    // 析构时释放队列中的连接（raw pointer），避免资源泄漏
    while (!_connectionQueue.empty()) {
        Connection *ptr = _connectionQueue.front();
        _connectionQueue.pop();
        delete ptr;
    }
}

bool ConnectionPool::LoadConfigFile() {
    // 从 JSON 文件加载配置；相对路径依赖于可执行文件的工作目录
    std::ifstream ifs("../../config/dbconf.json");
    json js;
    ifs >> js;
    std::cout << js << std::endl;
    if (!js.is_object()) {
        LOG_ERROR("JSON is NOT Object");
        return false;
    }
    // 字段类型校验，保证配置的健壮性
    if (!js["ip"].is_string() ||
        !js["port"].is_number() ||
        !js["user"].is_string() ||
        !js["pwd"].is_string() ||
        !js["db"].is_string() ||
        !js["minSize"].is_number() ||
        !js["maxSize"].is_number() ||
        !js["maxIdleTime"].is_number() ||
        !js["timeout"].is_number()) {
        LOG_ERROR("JSON The data type does not match");
        return false;
    }
    _ip = js["ip"].get<std::string>();
    _port = js["port"].get<uint16_t>();
    _user = js["user"].get<std::string>();
    _pwd = js["pwd"].get<std::string>();
    _db = js["db"].get<std::string>();
    _minSize = js["minSize"].get<size_t>();
    _maxSize = js["maxSize"].get<size_t>();
    _maxIdleTime = js["maxIdleTime"].get<size_t>();     // 秒
    _connectionTimeout = js["timeout"].get<size_t>();   // 微秒
    return true;
}

void ConnectionPool::ProduceConnectionTask() {
    // 生产者：在连接不足时创建新连接
    while (true) {
        std::unique_lock<std::mutex> lock(_mtx);
        // 当队列大小达到（或超过）_minSize 时，进入等待，避免无意义生产
        while (_connectionQueue.size() >= _minSize) {
            _cv.wait(lock);
        }
        // 容量未达上限则创建新连接
        if (_connectionCount < _maxSize) {
            AddConnection();
        }
        _cv.notify_all();
    }
}

void ConnectionPool::ScannerConnectionTask() {
    // 回收者：定时检查空闲连接并回收超时的连接
    while (true) {
        // 以 _maxIdleTime 为周期进行扫描（单位：秒）
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));
        std::unique_lock<std::mutex> lock(_mtx);
        // 仅在当前连接总数大于最小容量时尝试回收
        while (_connectionCount > _minSize) {
            // 队列近似按归还时间排序：队头最“老”，若它未超时，后续更“新”的也不会超时
            Connection *ptr = _connectionQueue.front();
            // 说明：GetAliveTime 返回微秒；此处比较阈值使用 _maxIdleTime * 1000（毫秒），
            // 若需要严格一致，可将比较统一为同单位
            if (ptr->GetAliveTime() >= _maxIdleTime * 1000) {
                _connectionQueue.pop();
                --_connectionCount;
                delete ptr;
            } else {
                break;
            }
        }
    }
}

void ConnectionPool::AddConnection() {
    // 新建连接并入队；刷新活跃时间作为闲置起点
    Connection *conn = new Connection();
    conn->Connect(_ip, _port, _user, _pwd, _db);
    conn->RefreshAliveTime();
    _connectionQueue.push(conn);
    ++_connectionCount;
}
