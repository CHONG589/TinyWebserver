// 文件说明：
// - 提供连接池的单例获取、连接获取（带自定义析构归还）、连接生产与空闲回收
// - 通过 JSON 文件加载数据库与池参数，初始化最小连接数，并启动后台守护线程
// - 关键并发原语：std::mutex + std::condition_variable 保护队列与线程协作
#include <fstream>

#include <json/json.h>
#include "db/ConnectionPool.h"

/**
 * @brief 获取连接池单例
 * 说明：C++11 之后静态局部变量初始化是线程安全的
 */
ConnectionPool &ConnectionPool::GetConnectionPool() {
    // C++11 线程安全的静态局部变量初始化，保证单例获取在并发场景下安全
    static ConnectionPool pool;
    return pool;
}

/**
 * @brief 获取一个可用连接（阻塞式）
 * - 当队列为空时，等待 m_connectionTimeout 时间；若超时仍为空则继续等待（重试）
 * - 返回值为 shared_ptr，析构时通过自定义删除器归还连接到队列，并刷新活跃时间
 */
std::shared_ptr<Connection> ConnectionPool::GetConnection() {
    // 消费者：获取连接
    // - 当队列为空时，等待 m_connectionTimeout（微秒）；若超时仍为空则继续等待（重试）
    // - 返回 shared_ptr<Connection>，自定义删除器在智能指针析构时归还连接并刷新活跃时间
    std::unique_lock<std::mutex> lock(m_mtx);
    while (m_connectionQueue.empty()) {  // 连接为空，就阻塞等待m_connectionTimeout时间，如果时间过了，还没唤醒
        if (std::cv_status::timeout == m_cv.wait_for(lock, std::chrono::microseconds(m_connectionTimeout))) {
            if (m_connectionQueue.empty()) { //就可能还是为空
                continue;
            }
        }
    }
    // 对于使用完成的连接，不能直接销毁该连接，而是需要将该连接归还给连接池的队列，供之后的其他消费者使用，
    // 于是我们使用智能指针，自定义其析构函数，完成放回的操作：
    std::shared_ptr<Connection> res(m_connectionQueue.front(), [&](Connection *conn) {
        std::unique_lock<std::mutex> locker(m_mtx);
        // 归还前刷新活跃时间，使得该连接在池中的空闲计时从“当前时刻”重新开始
        conn->RefreshAliveTime();
        m_connectionQueue.push(conn);
    });
    m_connectionQueue.pop();
    m_cv.notify_all();
    return res;
}

/**
 * @brief 构造函数
 * - 加载配置，初始化最小数量的连接
 * - 启动生产者线程与扫描回收线程（均为守护线程）
 */
ConnectionPool::ConnectionPool() {
    // 加载配置失败则不继续初始化
    if (!LoadConfigFile()) {
        LOG_ERROR() << "JSON Config Error";
        return;
    }
    // 创建初始数量的连接（维持不低于 _minSize）
    for (size_t i = 0; i < m_minSize; ++i) {
        AddConnection();
    }
    
    // 初始化退出标志
    m_isShutdown = false;

    // 启动一个新的线程，作为连接的生产者
    m_produceThread = std::thread(std::bind(&ConnectionPool::ProduceConnectionTask, this));
    
    // 启动一个新的定时线程，扫描超过 maxIdleTime 时间的空闲连接
    m_scannerThread = std::thread(std::bind(&ConnectionPool::ScannerConnectionTask, this));
}

ConnectionPool::~ConnectionPool() {
    // 设置退出标志并通知所有线程
    m_isShutdown = true;
    m_cv.notify_all();

    // 等待线程安全退出
    if (m_produceThread.joinable()) {
        m_produceThread.join();
    }
    if (m_scannerThread.joinable()) {
        m_scannerThread.join();
    }

    // 析构时释放队列中的连接（raw pointer），避免资源泄漏
    while (!m_connectionQueue.empty()) {
        Connection *ptr = m_connectionQueue.front();
        m_connectionQueue.pop();
        delete ptr;
    }
}

/**
 * @brief 加载 JSON 配置文件
 * @return 成功返回 true，失败返回 false
 * 说明：校验每个字段的类型有效性，并设置池参数与数据库连接信息
 */
bool ConnectionPool::LoadConfigFile() {
    // 从 JSON 文件加载配置；相对路径依赖于可执行文件的工作目录
    std::ifstream ifs("/home/zch/Project/TinyWebserver/config/db_config.json");
    if (!ifs.is_open()) {
        LOG_ERROR() << "open json file failed";
        return false;
    }
    
    Json::Reader reader;
    Json::Value js;
    if (!reader.parse(ifs, js)) {
        LOG_ERROR() << "JSON parse error";
        return false;
    }

    if (!js.isObject()) {
        LOG_ERROR() << "JSON is NOT Object";
        return false;
    }
    // 字段类型校验，保证配置的健壮性
    if (!js["ip"].isString() ||
        !js["port"].isIntegral() ||
        !js["user"].isString() ||
        !js["pwd"].isString() ||
        !js["db"].isString() ||
        !js["minSize"].isIntegral() ||
        !js["maxSize"].isIntegral() ||
        !js["maxIdleTime"].isIntegral() ||
        !js["timeout"].isIntegral()) {
        LOG_ERROR() << "JSON The data type does not match";
        return false;
    }
    m_ip = js["ip"].asString();
    m_port = js["port"].asUInt();
    m_user = js["user"].asString();
    m_pwd = js["pwd"].asString();
    m_db = js["db"].asString();
    m_minSize = js["minSize"].asUInt();
    m_maxSize = js["maxSize"].asUInt();
    m_maxIdleTime = js["maxIdleTime"].asUInt();     // 秒
    m_connectionTimeout = js["timeout"].asUInt();   // 微秒

    return true;
}

/**
 * @brief 连接生产者任务（独立线程）
 * - 当队列大小小于 m_minSize 且总连接数未达到 m_maxSize 时创建新连接
 * - 与消费者通过条件变量协作，避免忙等
 */
void ConnectionPool::ProduceConnectionTask() {
    // 生产者：在连接不足时创建新连接
    while (!m_isShutdown) {
        std::unique_lock<std::mutex> lock(m_mtx);
        // 当队列大小达到（或超过）_minSize 时，进入等待，避免无意义生产
        while (!m_isShutdown && m_connectionQueue.size() >= m_minSize) {
            m_cv.wait(lock);
        }
        
        // 如果是由于 shutdown 唤醒，则退出
        if (m_isShutdown) {
            break;
        }

        // 容量未达上限则创建新连接
        if (m_connectionCount < m_maxSize) {
            AddConnection();
        }
        m_cv.notify_all();
    }
}

/**
 * @brief 空闲连接扫描回收任务（独立线程）
 * - 每隔 m_maxIdleTime 秒检查队头连接的闲置时长，超过阈值则回收
 * - 队列按归还时间近似从旧到新；队头不超阈值时后续更“新”的连接也不会超
 * - 注意：GetAliveTime 返回微秒，这里比较使用 m_maxIdleTime * 1000 的单位（毫秒），
 *         如需严格一致可统一为同一单位（比如全部按微秒）
 */
void ConnectionPool::ScannerConnectionTask() {
    // 回收者：定时检查空闲连接并回收超时的连接
    while (!m_isShutdown) {
        // 以 _maxIdleTime 为周期进行扫描（单位：秒）
        // 使用 wait_for 替代 sleep，支持被 notify 唤醒以快速退出
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.wait_for(lock, std::chrono::seconds(m_maxIdleTime), [this]{ return m_isShutdown.load(); });
        
        if (m_isShutdown) {
            break;
        }

        // 仅在当前连接总数大于最小容量时尝试回收
        while (m_connectionCount > m_minSize) {
            // 队列近似按归还时间排序：队头最“老”，若它未超时，后续更“新”的也不会超时
            Connection *ptr = m_connectionQueue.front();
            // 说明：GetAliveTime 返回微秒；此处比较阈值使用 _maxIdleTime * 1000（毫秒），
            // 若需要严格一致，可将比较统一为同单位
            if (ptr->GetAliveTime() >= m_maxIdleTime * 1000) {
                m_connectionQueue.pop();
                --m_connectionCount;
                delete ptr;
            } else {
                break;
            }
        }
    }
}

/**
 * @brief 创建并入队一个新连接
 * - 连接成功后刷新活跃时间并推入队列，增加 m_connectionCount
 */
void ConnectionPool::AddConnection() {
    // 新建连接并入队；刷新活跃时间作为闲置起点
    Connection *conn = new Connection();
    conn->Connect(m_ip, m_port, m_user, m_pwd, m_db);
    conn->RefreshAliveTime();
    m_connectionQueue.push(conn);
    ++m_connectionCount;
}
