/**
 * @file http_server.h
 * @brief HTTP服务器的封装
 * @author zch
 * @date 2025-04-11
 */

#ifndef HTTP_SERVER_H__
#define HTTP_SERVER_H__

#include <string>
#include <unordered_map>

#include "tcp_server.h"
#include "address.h"
#include "socket.h"
#include "pool/sqlconnpool.h"
#include "http/httpconn.h"

const int MAX_FD = 65536;

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = true
               , IOManager *worker = IOManager::GetThis()
               , IOManager *io_worker = IOManager::GetThis()
               , IOManager *accept_worker = IOManager::GetThis());

    ~HttpServer();
    // void setUser(const std::string &user) { m_user = user; }
    // void setPassword(const std::string &password) { m_password = password; }
    // void setDatabaseName(const std::string &db_name) { m_database_name = db_name; }

protected:
    void handleClient(Socket::ptr client) override;

private:
    bool m_isKeepalive;
    std::unordered_map<int, HttpConn> users_;
    // string m_user;
    // string m_password;
    // string m_database_name;
};

#endif
