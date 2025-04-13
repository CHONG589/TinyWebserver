/**
 * @file socket.h
 * @brief Socket 封装
 * @author zch
 * @date 2025-03-29
 */

#ifndef SOCKET_H__
#define SOCKET_H__

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "address.h"
#include "noncopyable.h"

/**
 * @brief Socket 封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    //Socket 类型
    enum Type { TCP = SOCK_STREAM, UDP = SOCK_DGRAM };
    //Socket 协议簇
    enum Family { IPv4 = AF_INET, IPv6 = AF_INET6, UNIX = AF_UNIX };

    Socket(int family, int type, int protocol = 0);
    virtual ~Socket();
    //address 中有 family，即创建 family(IPv4或IVv6等)，IP 地址已确定
    //即根据address来确定的
    static Socket::ptr CreateTCP(Address::ptr address);
    static Socket::ptr CreateUDP(Address::ptr address);
    //创建 IPv4 的 TCP Socket，不是根据 address 来创建
    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();
    //创建 IPv6 的 TCP Socket
    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();
    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

    //设置 sockopt @see setsockopt
    bool setOption(int level, int option, const void *result, socklen_t len);
    //设置 sockopt 模板
    template <class T>
    bool setOption(int level, int option, const T &value) {
        return setOption(level, option, &value, sizeof(T));
    }
    //获取 sockopt，即类似 getsockopt 的系统调用
    bool getOption(int level, int option, void *result, socklen_t *len);

    //获取发送超时时间
    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);
    //获取接受超时时间
    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    virtual bool bind(const Address::ptr addr);
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    virtual bool reconnect(uint64_t timeout_ms = -1);
    virtual bool listen(int backlog = SOMAXCONN);
    virtual Socket::ptr accept();
    //返回值大于零表示发送成功的数据大小
    //等于零表示 socket 被关闭
    //小于零表示 socket 出错
    virtual int send(const void *buffer, size_t length, int flags = 0);
    virtual int send(const iovec *buffers, size_t length, int flags = 0);
    virtual int sendTo(const void *buffer, size_t length, const Address::ptr to, int flags = 0);
    virtual int sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags = 0);
    virtual int recv(void *buffer, size_t length, int flags = 0);
    virtual int recv(iovec *buffers, size_t length, int flags = 0);
    virtual int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);
    virtual int recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags = 0);
    virtual bool close();

    bool isValid() const;
    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();
    int getSocket() const { return m_sock; }
    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }
    bool isConnected() const { return m_isConnected; }
    int getError();

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

protected:
    void initSock();
    void newSock();
    //在 accept 返回的新 sock时，要对这个新 sock 进行初始化成
    //socket 对象。以及要对这个 fd 进行初始化成 fd 对象。
    virtual bool init(int sock);

protected:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};

#endif
 