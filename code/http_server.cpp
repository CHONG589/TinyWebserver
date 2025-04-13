#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "http_server.h"
#include "log/log.h"

HttpServer::HttpServer(bool keepalive
                     , IOManager *worker
                     , IOManager *io_worker
                     , IOManager *accept_worker)
                     : TcpServer(io_worker, accept_worker)
                     , m_isKeepalive(keepalive) {
    
    char *srcDir = getcwd(nullptr, 256);
    char *logDir = getcwd(nullptr, 256); 
    strcat(logDir, "/log");
    Log::Instance()->init(1, logDir, ".log", 1024);
    assert(srcDir);
    strcat(srcDir, "/resources");
    LOG_INFO("srcDir: %s", srcDir);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir;
    SqlConnPool::Instance()->Init("localhost", 3306, "zch", "589520", "yourdb", 12);
}

HttpServer::~HttpServer() {
    SqlConnPool::Instance()->ClosePool();
}

void HttpServer::handleClient(Socket::ptr client) {
    int client_socket = client->getSocket();
    if(!client->isValid()) {
        client->close();
        return ;
    }
    sockaddr_in *addr = (sockaddr_in *)(client->getRemoteAddress()->getAddr());
    users_[client_socket].init(client_socket, *addr);
    while(client->isConnected()) {
        int errnoNum = 0;
        int readNum = users_[client_socket].read(&errnoNum);
        if(readNum <= 0) {
            if(errno == EINTR) {
                continue ;
            }
            client->close();
            return ;
        }
        if(users_[client_socket].process()) {
            users_[client_socket].write(&errnoNum);
            break;
        }
    }
    client->close();
}
