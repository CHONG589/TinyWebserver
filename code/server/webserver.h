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

#include "epoller.h"
#include "../timer/heaptimer.h"
#include "../coroutine/iomanager.h"

#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"

#include "../http/httpconn.h"

class WebServer {
public:
    /**
     * 主要做初始化日志，数据连接池，配置事件模式，创建 epoll 对象
     * 等，以及它的一系列操作（setsockopt,bind,listen,还有增加监
     * 听事件）。设置非阻塞。
     */
    WebServer(
        int port, int trigMode, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    void Start();

private:
    bool InitSocket_(); 
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);
  
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    int port_;
    bool openLinger_;
    int timeoutMS_;  /* 毫秒MS */
    //bool isClose_;
    int listenFd_;
    char* srcDir_;
    
    uint32_t listenEvent_;  // 监听事件
    uint32_t connEvent_;    // 连接事件
   
    zch::IOManager *iom_;
    std::unique_ptr<HeapTimer> timer_;
    //std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    // 作用：保存所有连接对象，key为fd，value为HttpConn对象。
    std::unordered_map<int, HttpConn> users_;
};


#endif //WEBSERVER_H
