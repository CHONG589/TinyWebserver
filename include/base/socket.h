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
#include "zchlog.h"

/**
 * @brief Socket 封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /**
     * @brief Socket 类型
     */
    enum Type { TCP = SOCK_STREAM, UDP = SOCK_DGRAM };

    /**
     * @brief Socket 协议簇
     */
    enum Family { IPv4 = AF_INET, IPv6 = AF_INET6, UNIX = AF_UNIX };

    /**
     * @brief 构造函数
     * @param[in] family 协议簇
     * @param[in] type 类型
     * @param[in] protocol 协议
     */
    Socket(int family, int type, int protocol = 0);

    /**
     * @brief 析构函数
     */
    virtual ~Socket();

    /**
     * @brief 创建 TCP Socket
     * @param[in] address 地址
     * @return Socket::ptr
     */
    static Socket::ptr CreateTCP(Address::ptr address);

    /**
     * @brief 创建 UDP Socket
     * @param[in] address 地址
     * @return Socket::ptr
     */
    static Socket::ptr CreateUDP(Address::ptr address);

    /**
     * @brief 创建 IPv4 的 TCP Socket
     * @return Socket::ptr
     */
    static Socket::ptr CreateTCPSocket();

    /**
     * @brief 创建 IPv4 的 UDP Socket
     * @return Socket::ptr
     */
    static Socket::ptr CreateUDPSocket();

    /**
     * @brief 创建 IPv6 的 TCP Socket
     * @return Socket::ptr
     */
    static Socket::ptr CreateTCPSocket6();

    /**
     * @brief 创建 IPv6 的 UDP Socket
     * @return Socket::ptr
     */
    static Socket::ptr CreateUDPSocket6();

    /**
     * @brief 创建 Unix TCP Socket
     * @return Socket::ptr
     */
    static Socket::ptr CreateUnixTCPSocket();

    /**
     * @brief 创建 Unix UDP Socket
     * @return Socket::ptr
     */
    static Socket::ptr CreateUnixUDPSocket();

    /**
     * @brief 设置 sockopt
     * @see setsockopt
     * @param[in] level 级别
     * @param[in] option 选项
     * @param[in] result 值
     * @param[in] len 长度
     * @return bool 是否成功
     */
    bool setOption(int level, int option, const void *result, socklen_t len);

    /**
     * @brief 设置 sockopt 模板
     * @param[in] level 级别
     * @param[in] option 选项
     * @param[in] value 值
     * @return bool 是否成功
     */
    template <class T>
    bool setOption(int level, int option, const T &value) {
        return setOption(level, option, &value, sizeof(T));
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
    bool getOption(int level, int option, void *result, socklen_t *len);

    /**
     * @brief 获取发送超时时间(毫秒)
     * @return int64_t
     */
    int64_t getSendTimeout();

    /**
     * @brief 设置发送超时时间(毫秒)
     * @param[in] v 毫秒
     */
    void setSendTimeout(int64_t v);

    /**
     * @brief 获取接受超时时间(毫秒)
     * @return int64_t
     */
    int64_t getRecvTimeout();

    /**
     * @brief 设置接受超时时间(毫秒)
     * @param[in] v 毫秒
     */
    void setRecvTimeout(int64_t v);

    /**
     * @brief 绑定地址
     * @param[in] addr 地址
     * @return bool 是否成功
     */
    virtual bool bind(const Address::ptr addr);

    /**
     * @brief 连接地址
     * @param[in] addr 地址
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return bool 是否成功
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    /**
     * @brief 重连
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return bool 是否成功
     */
    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 监听
     * @param[in] backlog 队列长度
     * @return bool 是否成功
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 接受连接
     * @return Socket::ptr
     */
    virtual Socket::ptr accept();

    /**
     * @brief 发送数据
     * @param[in] buffer 数据
     * @param[in] length 长度
     * @param[in] flags 标志
     * @return int 发送字节数, >0成功, =0关闭, <0出错
     */
    virtual int send(const void *buffer, size_t length, int flags = 0);

    /**
     * @brief 发送数据(iovec)
     * @param[in] buffers 数据
     * @param[in] length 长度
     * @param[in] flags 标志
     * @return int 发送字节数
     */
    virtual int send(const iovec *buffers, size_t length, int flags = 0);

    /**
     * @brief 发送数据到指定地址
     * @param[in] buffer 数据
     * @param[in] length 长度
     * @param[in] to 目标地址
     * @param[in] flags 标志
     * @return int 发送字节数
     */
    virtual int sendTo(const void *buffer, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 发送数据到指定地址(iovec)
     * @param[in] buffers 数据
     * @param[in] length 长度
     * @param[in] to 目标地址
     * @param[in] flags 标志
     * @return int 发送字节数
     */
    virtual int sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 接收数据
     * @param[out] buffer 数据
     * @param[in] length 长度
     * @param[in] flags 标志
     * @return int 接收字节数
     */
    virtual int recv(void *buffer, size_t length, int flags = 0);

    /**
     * @brief 接收数据(iovec)
     * @param[out] buffers 数据
     * @param[in] length 长度
     * @param[in] flags 标志
     * @return int 接收字节数
     */
    virtual int recv(iovec *buffers, size_t length, int flags = 0);

    /**
     * @brief 接收数据从指定地址
     * @param[out] buffer 数据
     * @param[in] length 长度
     * @param[out] from 来源地址
     * @param[in] flags 标志
     * @return int 接收字节数
     */
    virtual int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief 接收数据从指定地址(iovec)
     * @param[out] buffers 数据
     * @param[in] length 长度
     * @param[out] from 来源地址
     * @param[in] flags 标志
     * @return int 接收字节数
     */
    virtual int recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief 关闭 socket
     * @return bool 是否成功
     */
    virtual bool close();

    /**
     * @brief 是否有效
     * @return bool
     */
    bool isValid() const;

    /**
     * @brief 获取远程地址
     * @return Address::ptr
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址
     * @return Address::ptr
     */
    Address::ptr getLocalAddress();

    /**
     * @brief 获取 socket 句柄
     * @return int
     */
    int getSocket() const { return m_sock; }

    /**
     * @brief 获取协议簇
     * @return int
     */
    int getFamily() const { return m_family; }

    /**
     * @brief 获取类型
     * @return int
     */
    int getType() const { return m_type; }

    /**
     * @brief 获取协议
     * @return int
     */
    int getProtocol() const { return m_protocol; }

    /**
     * @brief 是否连接
     * @return bool
     */
    bool isConnected() const { return m_isConnected; }

    /**
     * @brief 获取错误码
     * @return int
     */
    int getError();

    /**
     * @brief 取消读事件
     * @return bool
     */
    bool cancelRead();

    /**
     * @brief 取消写事件
     * @return bool
     */
    bool cancelWrite();

    /**
     * @brief 取消 accept 事件
     * @return bool
     */
    bool cancelAccept();

    /**
     * @brief 取消所有事件
     * @return bool
     */
    bool cancelAll();

protected:
    /**
     * @brief 初始化 socket
     */
    void initSock();

    /**
     * @brief 创建新 socket
     */
    void newSock();

    /**
     * @brief 初始化 socket
     * @param[in] sock socket句柄
     * @return bool
     */
    virtual bool init(int sock);

protected:
    // socket句柄
    int m_sock;
    // 协议簇
    int m_family;
    // 类型
    int m_type;
    // 协议
    int m_protocol;
    // 是否连接
    bool m_isConnected;
    // 本地地址
    Address::ptr m_localAddress;
    // 远程地址
    Address::ptr m_remoteAddress;
};

#endif
