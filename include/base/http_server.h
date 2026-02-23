/**
 * @file http_server.h
 * @brief HTTP服务器的封装
 * @author zch
 * @date 2025-04-11
 */

#ifndef HTTP_SERVER_H__
#define HTTP_SERVER_H__

#include <string>

#include "tcp_server.h"
#include "address.h"
#include "socket.h"
#include "http/httpconn.h"
#include "zchlog.h"

const int MAX_FD = 65536;

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] keepalive 是否保持连接
     * @param[in] worker 工作调度器
     * @param[in] io_worker IO调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool keepalive = true
               , IOManager *worker = IOManager::GetThis()
               , IOManager *io_worker = IOManager::GetThis()
               , IOManager *accept_worker = IOManager::GetThis());

    /**
     * @brief 析构函数
     */
    ~HttpServer();
    // void setUser(const std::string &user) { m_user = user; }
    // void setPassword(const std::string &password) { m_password = password; }
    // void setDatabaseName(const std::string &db_name) { m_database_name = db_name; }

    /**
     * @brief 处理客户端连接
     * @param[in] client 客户端Socket
     */
    void handleClient(Socket::ptr client) override;

    /**
     * @brief 开始接受连接
     * @param[in] sock 服务器Socket
     */
    void startAccept(Socket::ptr sock) override;

private:
    // 是否保持连接
    bool m_isKeepalive;
    // string m_user;
    // string m_password;
    // string m_database_name;
};

#endif
