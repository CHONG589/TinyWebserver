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

#include "address.h"
#include "./coroutine/noncopyable.h"

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

    //获取发送超时时间
    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);
    //获取接受超时时间
    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);
    //获取 sockopt，即类似 getsockopt 的系统调用
    bool getOption(int level, int option, void *result, socklen_t *len);

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
 