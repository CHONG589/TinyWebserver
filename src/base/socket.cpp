#include "base/socket.h"
#include "coroutine/iomanager.h"
#include "base/fd_manager.h"
// #include "hook.h"

/**
 * @brief 构造函数
 * @param[in] family 协议簇
 * @param[in] type 类型
 * @param[in] protocol 协议
 */
Socket::Socket(int family, int type, int protocol)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected(false) {
}

/**
 * @brief 关闭 socket
 * @return bool 是否成功
 */
bool Socket::close() {
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

/**
 * @brief 析构函数
 */
Socket::~Socket() {
    close();
}

/**
 * @brief 创建 TCP Socket
 * @param[in] address 地址
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateTCP(Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

/**
 * @brief 创建 UDP Socket
 * @param[in] address 地址
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateUDP(Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * @brief 创建 IPv4 的 TCP Socket
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

/**
 * @brief 创建 IPv4 的 UDP Socket
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * @brief 创建 IPv6 的 TCP Socket
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

/**
 * @brief 创建 IPv6 的 UDP Socket
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * @brief 创建 Unix TCP Socket
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

/**
 * @brief 创建 Unix UDP Socket
 * @return Socket::ptr
 */
Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

/**
 * @brief 获取发送超时时间(毫秒)
 * @return int64_t
 */
int64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        // SO_SNDTIMEO 写超时，SO_RCVTIMEO 读超时
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

/**
 * @brief 设置 sockopt
 * @see setsockopt
 * @param[in] level 级别
 * @param[in] option 选项
 * @param[in] result 值
 * @param[in] len 长度
 * @return bool 是否成功
 */
bool Socket::setOption(int level, int option, const void *result, socklen_t len) {
    if (setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        LOG_WARN() << "setOption(" << level << ", " << option << ") error: " << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 获取 sockopt
 * @see getsockopt
 * @param[in] level 级别
 * @param[in] option 选项
 * @param[out] result 值
 * @param[in] len 长度
 * @return bool 是否成功
 */
bool Socket::getOption(int level, int option, void *result, socklen_t *len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t *)len);
    if (rt) {
        LOG_WARN() << "getOption(" << level << ", " << option << ") error: " << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 设置发送超时时间(毫秒)
 * @param[in] v 毫秒
 */
void Socket::setSendTimeout(int64_t v) {
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/**
 * @brief 获取接受超时时间(毫秒)
 * @return int64_t
 */
int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

/**
 * @brief 设置接受超时时间(毫秒)
 * @param[in] v 毫秒
 */
void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

/**
 * @brief 绑定地址
 * @param[in] addr 地址
 * @return bool 是否成功
 */
bool Socket::bind(const Address::ptr addr) {
    m_localAddress = addr;
    if(!isValid()) {
        newSock();
        if(!isValid()) {
            return false;
        }
    }
    
    if(addr->getFamily() != m_family) {
        LOG_ERROR() << "bind sock.family and addr.family not equal!";
        return false;
    }

    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        LOG_ERROR() << "bind error: " << strerror(errno) << " addr=" << addr->toString();
        return false;
    }
    //防止一开始传进来的addr为nullptr
    getLocalAddress();
    return true;
}

/**
 * @brief 连接地址
 * @param[in] addr 地址
 * @param[in] timeout_ms 超时时间(毫秒)
 * @return bool 是否成功
 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;
    if(!isValid()) {
        newSock();
        if(!isValid()) {
            return false;
        }
    }
    if(addr->getFamily() != m_family) {
        LOG_ERROR() << "connect sock.family and addr.family not equal!";
        return false;
    }

    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            LOG_ERROR() << "connect error: " << strerror(errno) << " addr=" << addr->toString();
            close();
            return false;
        }
    }
    else {
        // if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
        //     LOG_ERROR("connect timeout: %c!!!", strerror(errno));
        //     close();
        //     return false;
        // }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

/**
 * @brief 重连
 * @param[in] timeout_ms 超时时间(毫秒)
 * @return bool 是否成功
 */
bool Socket::reconnect(uint64_t timeout_ms) {
    if(!m_remoteAddress) {
        LOG_ERROR() << "reconnect m_remoteAddress is null";
        return false;
    }
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}

/**
 * @brief 监听
 * @param[in] backlog 队列长度
 * @return bool 是否成功
 */
bool Socket::listen(int backlog) {
    if(!isValid()) {
        LOG_ERROR() << "listen error sock = -1";
        return false;
    }
    if(::listen(m_sock, backlog)) {
        LOG_ERROR() << "listen error: " << strerror(errno) << " backlog=" << backlog;
        return false;
    }
    return true;
}

/**
 * @brief 接受连接
 * @return Socket::ptr
 */
Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        if(errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR() << "accept error: " << strerror(errno) << " m_sock=" << m_sock;
        }
        return nullptr;
    }
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

/**
 * @brief 初始化 socket
 * @param[in] sock socket句柄
 * @return bool
 */
bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if(ctx &&!ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

/**
 * @brief 发送数据
 * @param[in] buffer 数据
 * @param[in] length 长度
 * @param[in] flags 标志
 * @return int 发送字节数, >0成功, =0关闭, <0出错
 */
int Socket::send(const void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

/**
 * @brief 发送数据(iovec)
 * @param[in] buffers 数据
 * @param[in] length 长度
 * @param[in] flags 标志
 * @return int 发送字节数
 */
int Socket::send(const iovec *buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief 发送数据到指定地址
 * @param[in] buffer 数据
 * @param[in] length 长度
 * @param[in] to 目标地址
 * @param[in] flags 标志
 * @return int 发送字节数
 */
int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to, int flags) {
    if (isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

/**
 * @brief 发送数据到指定地址(iovec)
 * @param[in] buffers 数据
 * @param[in] length 长度
 * @param[in] to 目标地址
 * @param[in] flags 标志
 * @return int 发送字节数
 */
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

/**
 * @brief 接收数据
 * @param[out] buffer 数据
 * @param[in] length 长度
 * @param[in] flags 标志
 * @return int 接收字节数
 */
int Socket::recv(void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

/**
 * @brief 接收数据(iovec)
 * @param[out] buffers 数据
 * @param[in] length 长度
 * @param[in] flags 标志
 * @return int 接收字节数
 */
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

/**
 * @brief 接收数据从指定地址
 * @param[out] buffer 数据
 * @param[in] length 长度
 * @param[out] from 来源地址
 * @param[in] flags 标志
 * @return int 接收字节数
 */
int Socket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags) {
    if (isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

/**
 * @brief 接收数据从指定地址(iovec)
 * @param[out] buffers 数据
 * @param[in] length 长度
 * @param[out] from 来源地址
 * @param[in] flags 标志
 * @return int 接收字节数
 */
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

/**
 * @brief 是否有效
 * @return bool
 */
bool Socket::isValid() const {
    return m_sock != -1;
}

/**
 * @brief 获取远程地址
 * @return Address::ptr
 */
Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch(m_family) {
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
    // 此函数是获取远端的地址
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        LOG_ERROR() << "getpeername error: " << strerror(errno) << " m_sock=" << m_sock;
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

/**
 * @brief 获取本地地址
 * @return Address::ptr
 */
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
    //此函数是获取本地地址
    if (getsockname(m_sock, result->getAddr(), &addrlen)) {
        LOG_ERROR() << "getsockname error: " << strerror(errno) << " m_sock=" << m_sock;
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}

/**
 * @brief 获取错误码
 * @return int
 */
int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

/**
 * @brief 取消读事件
 * @return bool
 */
bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
}

/**
 * @brief 取消写事件
 * @return bool
 */
bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::WRITE);
}

/**
 * @brief 取消 accept 事件
 * @return bool
 */
bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
}

/**
 * @brief 取消所有事件
 * @return bool
 */
bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sock);
}

/**
 * @brief 初始化 socket
 */
void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

/**
 * @brief 创建新 socket
 */
void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if (m_sock != -1) {
        initSock();
    } else {
        LOG_ERROR() << "socket() error: " << strerror(errno);
    }
}
