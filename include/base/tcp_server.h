/**
 * @file tcp_server.h
 * @brief TCP服务器的封装
 * @author zch
 * @date 2025-04-11
 */

#ifndef TCP_SERVER_H__
#define TCP_SERVER_H__

#include <memory>
#include <functional>
#include <unordered_map>

#include "address.h"
#include "socket.h"
#include "noncopyable.h"
#include "http/httpconn.h"
#include "coroutine/iomanager.h"
#include "zchlog.h"

/**
 * @brief TCP服务器封装类
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] io_worker socket工作的调度器
     * @param[in] accept_worker 服务器socket接收连接的调度器
     */
    TcpServer(IOManager *io_worker = IOManager::GetThis()
              , IOManager *accept_worker = IOManager::GetThis());

    /**
     * @brief 析构函数
     */
    virtual ~TcpServer();

    /**
     * @brief 绑定地址
     * @param[in] addr 需要绑定的地址
     * @return bool 是否绑定成功
     */
    virtual bool bind(Address::ptr addr);

    /**
     * @brief 绑定地址数组
     * @param[in] addrs 需要绑定的地址数组
     * @param[out] fails 绑定失败的地址
     * @return bool 是否绑定成功
     */
    virtual bool bind(const std::vector<Address::ptr> &addrs
                      , std::vector<Address::ptr> &fails);

    /**
     * @brief 启动服务
     * @return bool 是否启动成功
     */
    virtual bool start();

    /**
     * @brief 停止服务
     */
    virtual void stop();

    /**
     * @brief 获取读取超时时间(毫秒)
     * @return uint64_t 超时时间
     */
    uint64_t getRecvTimeout() const { return m_recvTimeout; }

    /**
     * @brief 获取服务器名称
     * @return std::string 服务器名称
     */
    std::string getName() const { return m_name; }

    /**
     * @brief 设置读取超时时间(毫秒)
     * @param[in] v 超时时间
     */
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

    /**
     * @brief 设置服务器名称
     * @param[in] v 服务器名称
     */
    virtual void setName(const std::string &v) { m_name = v; }

    /**
     * @brief 服务是否停止
     * @return bool 是否停止
     */
    bool isStop() const { return m_isStop; }

    /**
     * @brief 处理新连接的socket类
     * @param[in] client 新连接的socket
     */
    virtual void handleClient(Socket::ptr client);

    /**
     * @brief 开始接受连接
     * @param[in] sock 服务器socket
     */
    virtual void startAccept(Socket::ptr sock);

protected:
    // 监听socket数组
    std::vector<Socket::ptr> m_socks;
    // 新连接的socket工作的调度器
    IOManager *m_ioWorker;
    // 服务器socket接收连接的调度器
    IOManager *m_acceptWorker;
    // 接收超时时间(毫秒)
    uint64_t m_recvTimeout;
    // 服务器名称
    std::string m_name;
    // 服务器类型
    std::string m_type;
    // 服务是否停止
    bool m_isStop;
};

#endif
