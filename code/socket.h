/**
 * @file socket.h
 * @brief Socket封装
 * @date 2025-01-12
 */

#ifndef __ZCH_SOCKET_H__
#define __ZCH_SOCKET_H__

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "address.h"
#include "noncopyable.h"

namespace zch {

/**
 * @brief Socket封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /**
     * @brief Socket类型
     */
    enum Type {
        //TCP类型
        TCP = SOCK_STREAM,
        //UDP类型
        UDP = SOCK_DGRAM
    };

    /**
     * @brief Socket协议簇
     */
    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
    };

    /**
     * @brief Socket构造函数
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
     * @brief 创建TCP Socket(根据传入的address中的地址簇创建)
     * @param[in] address 地址
     */
    static Socket::ptr CreateTCP(zch::Address::ptr address);

    /**
     * @brief 创建UDP Socket(根据传入的address中的地址簇创建)
     * @param[in] address 地址
     */
    static Socket::ptr CreateUDP(zch::Address::ptr address);

    /**
     * @brief 创建IPv4的TCP Socket(创建指定的地址簇类型的Socket)
     */
    static Socket::ptr CreateTCPSocket();

    /**
     * @brief 创建IPv4的UDP Socket(创建指定的地址簇类型的Socket)
     */
    static Socket::ptr CreateUDPSocket();

    /**
     * @brief 创建IPv6的TCP Socket(创建指定的地址簇类型的Socket)
     */
    static Socket::ptr CreateTCPSocket6();

    /**
     * @brief 创建IPv6的UDP Socket(创建指定的地址簇类型的Socket)
     */
    static Socket::ptr CreateUDPSocket6();

    /**
     * @brief 获取发送超时时间(毫秒)
     */
    int64_t getSendTimeout();

    /**
     * @brief 设置发送超时时间(毫秒)
     */
    void setSendTimeout(int64_t v);

    /**
     * @brief 获取接受超时时间(毫秒)
     */
    int64_t getRecvTimeout();

    /**
     * @brief 设置接受超时时间(毫秒)
     */
    void setRecvTimeout(int64_t v);

    /**
     * @brief 获取sockopt @see getsockopt
     */
    bool getOption(int level, int option, void *result, socklen_t *len);

    /**
     * @brief 获取sockopt模板 @see getsockopt
     */
    template<class T>
    bool getOption(int level, int option, T &result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    /**
     * @brief 设置sockopt @see setsockopt
     */
    bool setOption(int level, int option, const void *result, socklen_t len);

    /**
     * @brief 设置sockopt模板 @see setsockopt
     */
    template <class T>
    bool setOption(int level, int option, const T &value) {
        return setOption(level, option, &value, sizeof(T));
    }

    /**
     * @brief 接收connect链接
     * @return 成功返回新连接的socket,失败返回nullptr
     * @pre Socket必须bind,listen 成功
     */
    virtual Socket::ptr accept();

    /**
     * @brief 绑定地址
     * @param[in] addr 地址
     * @return 是否绑定成功
     */
    virtual bool bind(const Address::ptr addr);

    /**
     * @brief 连接地址
     * @param[in] addr 目标地址
     * @param[in] timeout_ms 超时时间(毫秒)
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 监听socket
     * @param[in] backlog 未完成连接队列的最大长度
     * @return 返回监听是否成功
     * @pre 必须先bind成功
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 关闭socket
     */
    virtual bool close();

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的长度
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 发送成功对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int send(const void *buffer, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 发送成功对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int send(const iovec *buffers, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的长度
     * @param[in] to 发送的目标地址
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 发送成功对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int sendTo(const void *buffer, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     * @param[in] to 发送的目标地址
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 发送成功对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */    
    virtual int sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 接收到对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int recv(void *buffer, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffers 接收数据的内存(iovec数组)
     * @param[in] length 接收数据的内存大小(iovec数组长度)
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 接收到对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int recv(iovec *buffers, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[out] from 发送端地址
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 接收到对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffers 接收数据的内存(iovec数组)
     * @param[in] length 接收数据的内存大小(iovec数组长度)
     * @param[out] from 发送端地址
     * @param[in] flags 标志字
     * @return 
     *      @retval > 0 接收到对应大小的数据
     *      @retval = 0 socket被关闭
     *      @retval < 0 socket出错
     */
    virtual int recvFrom(iovec *buffers, size_t length, Address::ptr from, ing flags = 0);

    /**
     * @brief 获取远端地址
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址
     */
    Address::ptr getLocalAddress();

    /**
     * @brief 获取协议簇
     */
    int getFamily() const { return m_family; }

    /**
     * @brief 获取类型
     */
    int getType() const { return m_type; }

    /**
     * @brief 获取协议
     */
    int getProtocol() const { return m_protocol; }

    /**
     * @brief 返回是否连接
     */
    bool isConnected() const { return m_isConnected; }

    /**
     * @brief 是否有效(m_sock != -1)
     */
    bool isValid() const;

    /**
     * @brief 返回Socket错误
     */
    int getError();

    /**
     * @brief 输出信息到流中
     */
    virtual std::ostream &dump(std::ostream &os) const;

    virtual std::string toString() const;

    /**
     * @brief 返回socket句柄
     */
    int getSocket() const { return m_sock; }

    /**
     * @brief 取消读
     */
    bool cancelRead();

    /**
     * @brief 取消写
     */
    bool cancelWrite();

    /**
     * @brief 取消accept
     */
    bool cancelAccept();

    /**
     * @brief 取消所有事件
     */
    bool cancelAll();

protected:
    /**
     * @brief 对sock进行设置setoption
     */
    void initSock();

    /**
     * @brief 创建socket
     */
    void newSock();

    /**
     * @brief 初始化sock(Socket类)
     */
    virtual bool init(int sock);
    
protected:
    //socket 句柄
    int m_sock;
    //协议簇
    int m_family;
    //类型
    int m_type;
    //协议
    int m_protocol;
    //是否连接
    bool m_isConnected;
    //本地地址
    Address::ptr m_localAddress;
    //远端地址
    Address::ptr m_remoteAddress;
};

/**
 * @brief 流式输出socket
 * @param[in, out] os 输出流
 * @param[in] sock Socket类
 */
std::ostream &operator<<(std::ostream &os, const Socket &sock);

}//namespace zch

#endif

