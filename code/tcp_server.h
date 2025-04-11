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

#include "address.h"
#include "socket.h"
#include "coroutine/iomanager.h"
#include "coroutine/noncopyable.h"

class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(IOManager *io_worker = IOManager::GetThis()
              , IOManager *accept_worker = IOManager::GetThis());

    virtual ~TcpServer();
    virtual bool bind(Address::ptr addr);
    //绑定地址数组，addrs为需要绑定的地址数组，fails绑定
    //失败的地址
    virtual bool bind(const std::vector<Address::ptr> &addrs
                      , std::vector<Address::ptr> &fails);

    //启动服务，里面需要bind，然后执行，即调用 schedule
    virtual bool start();
    virtual void stop();
    //返回读取超时时间(毫秒)
    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    //设置读取超时时间(毫秒)
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    virtual void setName(const std::string &v) { m_name = v; }
    bool isStop() const { return m_isStop; }

protected:
    //处理新连接的socket类
    virtual void handleClient(Socket::ptr client);
    //开始接受连接
    virtual void startAccept(Socket::ptr sock);

protected:
    //监听socket数组
    std::vector<Socket::ptr> m_socks;
    //新连接的socket工作的调度器
    IOManager *m_ioWorker;
    //服务区socket接收连接的调度器
    IOManager *m_acceptWorker;
    //接收超时时间(毫秒)
    uint64_t m_recvTimeout;
    //服务器名称
    std::string m_name;
    //服务器类型
    std::string m_type;
    //服务是否停止
    bool m_isStop;
};

#endif
