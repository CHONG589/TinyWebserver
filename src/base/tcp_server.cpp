#include "base/tcp_server.h"
#include "base/http_server.h"
#include "base/config.h"

static zch::Logger::ptr g_logger = LOG_NAME("system");

static zch::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    zch::Config::Lookup("server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");

/**
 * @brief 构造函数
 * @param[in] io_worker socket工作的调度器
 * @param[in] accept_worker 服务器socket接收连接的调度器
 */
TcpServer::TcpServer(IOManager *io_worker, IOManager *accept_worker)
    : m_ioWorker(io_worker)
    , m_acceptWorker(accept_worker)
    , m_recvTimeout(g_tcp_server_read_timeout->GetValue())
    , m_name("zch/1.0.0")
    , m_type("tcp")
    , m_isStop(true) {
}

/**
 * @brief 析构函数
 */
TcpServer::~TcpServer() {
    for (auto &i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

/**
 * @brief 绑定地址
 * @param[in] addr 需要绑定的地址
 * @return bool 是否绑定成功
 */
bool TcpServer::bind(Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

/**
 * @brief 绑定地址数组
 * @param[in] addrs 需要绑定的地址数组
 * @param[out] fails 绑定失败的地址
 * @return bool 是否绑定成功
 */
bool TcpServer::bind(const std::vector<Address::ptr> &addrs
                        , std::vector<Address::ptr> &fails) {
    for(auto &addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            LOG_ERROR(g_logger) << "bind fail errno = " << errno << ", errstr = " << strerror(errno);
            fails.push_back(addr);
            continue;
        }

        if(!sock->listen()) {
            LOG_ERROR(g_logger) << "listen fail errno = " << errno << ", errstr = " << strerror(errno);
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }

    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto &i : m_socks) {
        LOG_INFO(g_logger) << "type=" << m_type << " name=" << m_name << " server bind success: " << *i->getLocalAddress();
    }
    return true;
}

/**
 * @brief 启动服务
 * @return bool 是否启动成功
 */
bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }

    m_isStop = false;
    for(auto &sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }

    return true;
}

/**
 * @brief 停止服务
 */
void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

/**
 * @brief 处理新连接的socket类
 * @param[in] client 新连接的socket
 */
void TcpServer::handleClient(Socket::ptr client) {
    LOG_DEBUG(g_logger) << "handleClient: " << client->getSocket();
}

/**
 * @brief 开始接受连接
 * @param[in] sock 服务器socket
 */
void TcpServer::startAccept(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        }
    }
}
