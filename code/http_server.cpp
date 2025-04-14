#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "http_server.h"
#include "tcp_server.h"
#include "log/log.h"

std::unordered_map<int, HttpConn> users_;

HttpServer::HttpServer(bool keepalive
                     , IOManager *worker
                     , IOManager *io_worker
                     , IOManager *accept_worker)
                     : TcpServer(io_worker, accept_worker)
                     , m_isKeepalive(keepalive) {
    
    char *srcDir = getcwd(nullptr, 256);
    char *logDir = getcwd(nullptr, 256); 
    strcat(logDir, "/log");
    Log::Instance()->init(0, logDir, ".log", 1024);
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
    int errnoNum = 0;

    users_[client_socket].read(&errnoNum);
    users_[client_socket].process();
    users_[client_socket].write(&errnoNum);

    // m_ioWorker->addEvent(client_socket, IOManager::READ
    //     , std::bind(&HttpServer::handleClient, shared_from_this(), client));
    m_ioWorker->addEvent(client_socket, IOManager::READ
        , std::bind(&HttpServer::handleClient, client, std::placeholders::_1));
}

void HttpServer::startAccept(Socket::ptr sock) {

    Socket::ptr client = sock->accept();
    if(client) {
        int client_socket = client->getSocket();
        sockaddr_in *addr = (sockaddr_in *)(client->getRemoteAddress()->getAddr());
        users_[client_socket].init(client_socket, *addr);
        
        client->setRecvTimeout(sock->getRecvTimeout());
        // m_ioWorker->addEvent(client_socket, IOManager::READ
        //     , std::bind(&HttpServer::handleClient, shared_from_this(), client));
        m_ioWorker->addEvent(client_socket, IOManager::READ
            , std::bind(&HttpServer::handleClient, client, std::placeholders::_1));
        // m_ioWorker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
    }
    else {
        LOG_ERROR("accept errno = %d, errstr = %s", errno, strerror(errno));
    }
}
