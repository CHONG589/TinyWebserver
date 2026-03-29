// 文件说明：
// - 提供连接池的单例获取、连接获取（带自定义析构归还）、连接生产与空闲回收
// - 通过 JSON 文件加载数据库与池参数，初始化最小连接数，并启动后台守护线程
// - 关键并发原语：std::mutex + std::condition_variable 保护队列与线程协作
#include <fstream>

#include "db/ConnectionPool.h"

static zch::Logger::ptr g_logger = LOG_NAME("system");

static zch::ConfigVar<std::string>::ptr g_db_ip =
    zch::Config::Lookup("database.ip", std::string("127.0.0.1"), "database ip address");

static zch::ConfigVar<uint16_t>::ptr g_db_port =
    zch::Config::Lookup("database.port", (uint16_t)(3306), "database port");

static zch::ConfigVar<std::string>::ptr g_db_user =
    zch::Config::Lookup("database.user", std::string(""), "database user");

static zch::ConfigVar<std::string>::ptr g_db_pwd =
    zch::Config::Lookup("database.pwd", std::string(""), "database passwd");

static zch::ConfigVar<std::string>::ptr g_db_name =
    zch::Config::Lookup("database.db", std::string(""), "database name");

static zch::ConfigVar<size_t>::ptr g_db_min_size =
    zch::Config::Lookup("database.minsize", (size_t)(100), "connectPool min size");

static zch::ConfigVar<size_t>::ptr g_db_max_size =
    zch::Config::Lookup("database.maxsize", (size_t)(1024), "connectPool max size");

static zch::ConfigVar<size_t>::ptr g_db_max_idle_time =
    zch::Config::Lookup("database.maxidletime", (size_t)(5000), "connectPool max idle time");

static zch::ConfigVar<size_t>::ptr g_db_timeout =
    zch::Config::Lookup("database.timeout", (size_t)(1000), "connectPool timeout"); 

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

    m_ip = g_db_ip->GetValue();
    m_port = g_db_port->GetValue();
    m_user = g_db_user->GetValue();
    m_pwd = g_db_pwd->GetValue();
    m_db = g_db_name->GetValue();
    m_minSize = g_db_min_size->GetValue();
    m_maxSize = g_db_max_size->GetValue();
    m_maxIdleTime = g_db_max_idle_time->GetValue();
    m_connectionTimeout = g_db_timeout->GetValue();

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
        if (m_connectionCount < (int)m_maxSize) {
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
        while (m_connectionCount > (int)m_minSize) {
            // 队列近似按归还时间排序：队头最“老”，若它未超时，后续更“新”的也不会超时
            Connection *ptr = m_connectionQueue.front();
            // 说明：GetAliveTime 返回微秒；此处比较阈值使用 _maxIdleTime * 1000（毫秒），
            // 若需要严格一致，可将比较统一为同单位
            size_t aliveTime = ptr->GetAliveTime();
            if (aliveTime >= m_maxIdleTime * 1000) {
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
