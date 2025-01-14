#include <limits.h>

#include "socket.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "./log/log.h"
#include "macro.h"
#include "hook.h"

namespace zch {

Socket::Socket(int family, int type, int protocol)
                : m_sock(-1)
                , m_family(family)
                , m_type(type)
                , m_protocol(protocol)
                , m_isConnected(false) {
}

Socket::~Socket() {
    close();
}

Socket::ptr Socket::CreateTCP(zch::Address::ptr address) {
    //创建一个sock对象，将对应的值初始化，传入的地址中会有想要的信息，
    //注意：此时还只是socket对象，还不是真正的socket套接字，若需要真正
    //的创建需要调用newSock()方法。
    //由于TCP连接的建立需要三次握手，三次握手的过程是在调用connect()
    //或者accept()的时候发生的，而不是创建对象就发生的。
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(zch::Address::ptr address) {
    //由于UDP是无连接的，只要创建了对象就表示建立了连接，所以这里创建后
    //就newSock()和将m_isConnected设置为true。
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

//注意：上面两个是根据传入的address中的地址族来创建TCP或UDP的，而下面的四个
//函数是创建指定的地址族（IPv4或IPv6）的 TCP 或 UDP 套接字。

Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

int64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx)
        return ctx->getTimeout(SO_SNDTIMEO);
    return -1;
}

void Socket::setSendTimeout(int64_t v) {
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx)
        return ctx->getTimeout(SO_RCVTIMEO);
    return -1;
}

void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void *result, socklen_t *len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t *)len);
    if (rt) {
        LOG_ERROR("Socket::getOption error!");
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void *result, socklen_t len) {
    if (setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        LOG_ERROR("Socket::setOption error!");
        return false;
    }
    return true;
}

bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if (ctx && ctx->isSocket() && !ctx->isClosed()) {
        m_sock = sock;
        m_isConnected = true;
        initSocket();
        getRemoteAddress();
        getLocalAddress();
        return true;
    }
    return false;
}

Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if (newsock == -1) {
        LOG_ERROR("Socke::accept error!");
        return nullptr;
    }
    if (sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket:::bind(const Address::ptr addr) {
    m_localAddress = addr;
    if (!isValid()) {
        LOG_ERROR("listen socket is not valid!");
        return false;
    }
    if (addr->getFamily() != m_family) {
        LOG_ERROR("bind sock.family != addr.family!");
        return false;
    }

    if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        LOG_ERROR("bind error!");
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms) {
    if (!m_remoteAddress) {
        LOG_ERROR("reconnect m_remoteAddress is null!");
        return false;
    }
    //通常在连接建立时被设置，并且在连接关闭或重新连接时可能需要更新或重置。
    //避免冲突: 重置本地地址可以确保在重新连接时，没有使用之前的本地地址，
    //从而避免潜在的地址冲突或绑定问题。
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;
    if (!isValid()) {
        LOG_ERROR("connect socket is not valid!");
        return false;
    }
    if (addr->getFamily() != m_family) {
        LOG_ERROR("connect sock.family != addr.family!");
        return false;
    }
    if (timeout_ms == (uint64_t)-1) {
        if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            LOG_ERROR("connect error!");
            close();
            return false;
        }
    }
    else {
        if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            LOG_ERROR("connect error!");
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog) {
    if (!isValid()) {
        LOG_ERROR("listen socket is not valid!");
        return false;
    }
    if (::listen(m_sock, backlog)) {
        LOG_ERROR("listen error!");
        return false;
    }
    return true;
}

bool Socket::close() {
    if (!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if (m_sock != -1) {
        if (::close(m_sock) == 0) {
            m_sock = -1;
            return true;
        }
        else {
            return false;
        }
    }
}

int Socket::send(const void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::send(m_sock, buffer, length, false);
    }
    return -1;
}

int Socket::send(const iovec *buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to, int flags) {
    if (isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrlen());
    }
    return -1;
}

int Socket::sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov     = (iovec *)buffers;
        msg.msg_iovlen  = length;
        msg.msg_name    = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec *buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags) {
    if (isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov     = (iovec *)buffers;
        msg.msg_iovlen  = length;
        msg.msg_name    = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress() {
    if (m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch (m_family) {
    case AF_INET:
        result.reset(new IPv4Address());
        break;
    case AF_INET6:
        result.reset(new IPv6Address());
        break;
    case AF_UNIX:
        result.reset(new UnixAddress());
        break;
    default:
        result.reset(new UnknownAddress(m_family));
        break;
    }
    socklen_t addrlen = result->getAddrLen();
    //获取远端地址
    if (getpeername(m_sock, result->getAddr(), &addrlen)) {
        LOG_ERROR("getpeername error!");
        return nullptr;
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
    if (m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    switch (m_family) {
    case AF_INET:
        result.reset(new IPv4Address());
        break;
    case AF_INET6:
        result.reset(new IPv6Address());
        break;
    case AF_UNIX:
        result.reset(new UnixAddress());
        break;
    default:
        result.reset(new UnknownAddress(m_family));
        break;
    }
    socklen_t addrlen = result->getAddrLen();
    //获取本地地址
    if (getsockname(m_sock, result->getAddr(), &addrlen)) {
        LOG_ERROR("getsockname error!");
        return nullptr;
    }
    m_localAddress = result;
    return m_localAddress;
}

bool Socket::isValid() const {
    return m_sock != -1;
}

int Socket::getError() {
    int error     = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if (m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if (m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, zch::IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, zch::IOManager::WRITE);
}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, zch::IOManager::READ);
}

bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sock);
}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if (m_sock != -1) {
        initSock();
    } else {
        LOG_ERROR("create socket error!");
    }
}

std::ostream &operator<<(std::ostream &os, const Socket &sock) {
    return sock.dump(os);
}

}//namespace zch
