#include "tcp_server.h"
#include "log/log.h"
#include "http_server.h"

TcpServer::TcpServer(IOManager *io_worker, IOManager *accept_worker)
    : m_ioWorker(io_worker)
    , m_acceptWorker(accept_worker)
    , m_recvTimeout((uint64_t)(60 * 1000 * 2))
    , m_name("zch/1.0.0")
    , m_type("tcp")
    , m_isStop(true) {
}

TcpServer::~TcpServer() {
    for (auto &i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

bool TcpServer::bind(Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr> &addrs
                        , std::vector<Address::ptr> &fails) {
    for(auto &addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            LOG_ERROR("bind fail errno = %d, errstr = %s", errno, strerror(errno));
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()) {
            LOG_ERROR("listen fail errno = %d, errstr = %s", errno, strerror(errno));
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
        m_acceptWorker->addEvent(sock->getSocket(), IOManager::READ
            , std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto &i : m_socks) {
        LOG_DEBUG("type = %s, name = %s, server bind success", m_type.data(), m_name.data());
    }
    return true;
}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto &i : m_socks) {
        m_acceptWorker->schedule(std::bind(&HttpServer::startAccept, shared_from_this(), i));
    }
    return true;
}

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

void TcpServer::handleClient(Socket::ptr client) {
    LOG_INFO("handle client");
}

void TcpServer::startAccept(Socket::ptr sock) {
    
    // Socket::ptr client = sock->accept();
    // if(client) {
    //     int client_socket = client->getSocket();
    //     sockaddr_in *addr = (sockaddr_in *)(client->getRemoteAddress()->getAddr());
    //     users_[client_socket].init(client_socket, *addr);
        
    //     client->setRecvTimeout(m_recvTimeout);
    //     m_ioWorker->addEvent(client_socket, IOManager::READ
    //         , std::bind(&HttpServer::handleClient, this, client));
    //     // m_ioWorker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
    // }
    // else {
    //     LOG_ERROR("accept errno = %d, errstr = %s", errno, strerror(errno));
    // }
}
