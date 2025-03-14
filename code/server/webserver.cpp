#include "webserver.h"

using namespace std;

WebServer::WebServer(
            int port, int trigMode, 
            int sqlPort, const char* sqlUser, 
            const  char* sqlPwd, const char* dbName, 
            int connPoolNum, int threadNum, bool openLog, 
            int logLevel, int logQueSize): 
            port_(port), timer_(new HeapTimer()), 
            epoller_(new Epoller()) {

    // 是否打开日志标志
    if(openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        //if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        LOG_INFO("========== Server init ==========");
        LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", (listenEvent_ & EPOLLET ? "ET": "LT"), (connEvent_ & EPOLLET ? "ET": "LT"));
        LOG_INFO("LogSys level: %d", logLevel);
        LOG_INFO("srcDir: %s", HttpConn::srcDir);
        LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
    }

    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strcat(srcDir_, "/resources/");
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    // 初始化操作
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);  // 连接池单例的初始化

    iom_ = new zch::IOManager(threadNum, false);
    //iom_->schedule(std::bind(&WebServer::InitSocket_, this));
    InitSocket_();

    // 初始化事件和初始化socket(监听)
    //InitEventMode_(trigMode);
    LOG_INFO("========== Server init end ==========");
    // if(!InitSocket_()) { isClose_ = true;}
}

WebServer::~WebServer() {
    close(listenFd_);
    //isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
    delete iom_;
}

void WebServer::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;    // 检测socket关闭
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;     // EPOLLONESHOT由一个线程处理
    switch (trigMode) {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

// void WebServer::Start() {
//     int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
//     //if(!isClose_) { LOG_INFO("========== Server start =========="); }
//     while(!isClose_) {
//         if(timeoutMS_ > 0) {
//             // 获取下一次的超时等待事件(至少这个时间才会有用户过期，
//             // 每次关闭超时连接则需要有新的请求进来)
//             timeMS = timer_->GetNextTick();     
//         }
//         int eventCnt = epoller_->Wait(timeMS);
//         for(int i = 0; i < eventCnt; i++) {
//             /* 处理事件 */
//             int fd = epoller_->GetEventFd(i);
//             uint32_t events = epoller_->GetEvents(i);
//             if(fd == listenFd_) {
//                 DealListen_();
//             }
//             else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
//                 assert(users_.count(fd) > 0);
//                 CloseConn_(&users_[fd]);
//             }
//             else if(events & EPOLLIN) {
//                 assert(users_.count(fd) > 0);
//                 DealRead_(&users_[fd]);
//             }
//             else if(events & EPOLLOUT) {
//                 assert(users_.count(fd) > 0);
//                 DealWrite_(&users_[fd]);
//             } else {
//                 LOG_ERROR("Unexpected event");
//             }
//         }
//     }
// }

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    // iom_->delEvent(client->GetFd(), client->GetEvents());
    client->Close();
}

// 处理读事件，主要逻辑是将OnRead加入线程池的任务队列中
// void WebServer::DealRead_(HttpConn* client) {
//     assert(client);
//     ExtentTime_(client);
//     threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
// }

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    /**
     * 下面可以增加定时事件，即从 accept 中接收到 fd 后，如果
     * 超过一定时间没处理就关闭连接。同样使用 iom_ 增加，这里
     * 的定时时间可以绑定到每个fd事件上，然后这里增加定时事件
     * 后就会将时间赋值到 iom_ 的定时器上。
     */
    // if(timeoutMS_ > 0) {
    //     timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    // }
    assert(&users_[fd]);
    iom_->addEvent(fd, zch::IOManager::READ, std::bind(&WebServer::OnRead_, this, &users_[fd]));
    //epoller_->AddFd(fd, EPOLLIN | connEvent_);
    //SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

// 处理监听套接字，主要逻辑是accept新的套接字，并加入timer和epoller中
void WebServer::DealListen_() {
    LOG_INFO("start deallisten...");
    
    while(true) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) break;
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    }

    //iom_->schedule(std::bind(&WebServer::DealListen_, this));  
    LOG_INFO("end deallisten...");
}

// 处理写事件，主要逻辑是将OnWrite加入线程池的任务队列中
// void WebServer::DealWrite_(HttpConn* client) {
//     assert(client);
//     ExtentTime_(client);
//     threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
// }

// 重新设置超时时间
// void WebServer::ExtentTime_(HttpConn* client) {
//     assert(client);
//     if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
// }

void WebServer::OnRead_(HttpConn* client) {
    LOG_INFO("OnRead_ start...");
    assert(client);
    int ret = -1;
    int readErrno = 0;
    // 读取客户端套接字的数据，读到httpconn的读缓存区
    ret = client->read(&readErrno);         
    if(ret <= 0 && readErrno != EAGAIN) {   // 读异常就关闭客户端
        CloseConn_(client);
        return;
    }
    // 业务逻辑的处理（先读后处理）
    OnProcess(client);
    LOG_INFO("OnRead_ end...");
}

/* 处理读（请求）数据的函数 */
void WebServer::OnProcess(HttpConn* client) {
    // process 函数是 httpcon 类中的函数，它用来处理请求数据，
    // 即从客户端传过来的数据，解析出它的请求报文，然后根据请求
    // 报文中的请求方法、请求路径、请求参数等信息，将资源生成
    // 响应报文返回给客户端。
    if(client->process()) { 
        // 成功处理请求，准备写响应数据，因为 process 只是将响应报文写
        // 入到 httpconn 的写缓存区，所以这里需要将写缓存区的数据发送给
        // 客户端。所以这里增加相应的写事件，并将 OnWrite_ 加入线程池的任务队列中。
        // 要增加写事件，然后检测到写事件后，再调用 httpconn 的 write 写给 client。
        // epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);   
        iom_->addEvent(client->GetFd(), zch::IOManager::WRITE, std::bind(&WebServer::OnWrite_, this, client));
    } 
    else {
        // 处理请求失败，继续读取请求数据。
        // epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
        iom_->addEvent(client->GetFd(), zch::IOManager::READ, std::bind(&WebServer::OnRead_, this, client));
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    LOG_INFO("OnWrite_ start...");
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            // 保持连接，继续监测读事件
            iom_->addEvent(client->GetFd(), zch::IOManager::READ, std::bind(&WebServer::OnRead_, this, client));
            //epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN); 
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) { 
            // 缓冲区满了，继续传输，设置写事件。
            // epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            iom_->addEvent(client->GetFd(), zch::IOManager::WRITE, std::bind(&WebServer::OnWrite_, this, client));
            return;
        }
    }
    CloseConn_(client);
    LOG_INFO("OnWrite_ end...");
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    LOG_INFO("initsocket start...")

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    // 绑定
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 监听
    ret = listen(listenFd_, 8);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    //IOManager::GetThis()->addEvent(listenFd_, IOManager::READ, std::bind(&WebServer::DealListen_, this));
    iom_->schedule(std::bind(&WebServer::DealListen_, this));
    // if(ret) {
    //     LOG_ERROR("Add listen error!");
    //     close(listenFd_);
    //     return false;
    // } 
    LOG_INFO("Server port:%d", port_);
    return true;
}
