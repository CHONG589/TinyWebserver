#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../coroutine/iomanager.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(
        int port, //int trigMode, //int timeoutMS, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();

private:
    bool InitSocket_(); 
    void handle_accept();
    void SendError_(int fd, const char*info);
    void CloseConn_(HttpConn* client, IOManager::Event event);
    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    int port_;
    bool openLinger_;
    //int timeoutMS_;  /* 毫秒MS */
    bool isClose_;
    int listenFd_;
    char* srcDir_;
    
    uint32_t listenEvent_;  // 监听事件
    uint32_t connEvent_;    // 连接事件
   
    IOManager::ptr iom_;
    // 作用：保存所有连接对象，key为fd，value为HttpConn对象。
    std::unordered_map<int, HttpConn> users_;
};


#endif //WEBSERVER_H
