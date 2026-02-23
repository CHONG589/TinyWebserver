#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "base/http_server.h"
#include "base/tcp_server.h"

std::unordered_map<int, HttpConn> users_;

/**
 * @brief 构造函数
 * @param[in] keepalive 是否保持连接
 * @param[in] worker 工作调度器
 * @param[in] io_worker IO调度器
 * @param[in] accept_worker 接收连接调度器
 */
HttpServer::HttpServer(bool keepalive
                     , IOManager *worker
                     , IOManager *io_worker
                     , IOManager *accept_worker)
                     : TcpServer(io_worker, accept_worker)
                     , m_isKeepalive(keepalive) {
    
    // 获取当前可执行文件路径的目录
    char *srcDir = getcwd(nullptr, 256);
    assert(srcDir);
    std::string rootDir(srcDir);
    
    // 如果是在 bin 目录下运行，则资源目录在上一级
    size_t pos = rootDir.find("/bin");
    if (pos != std::string::npos) {
        rootDir = rootDir.substr(0, pos);
    }
    
    std::string resources = rootDir + "/resources";
    
    // 分配内存并复制路径（注意：HttpConn::srcDir 是 const char*，需要持久的内存）
    // 这里简单处理，使用静态或堆分配。由于 HttpConn::srcDir 是静态指针，
    // 我们最好让它指向一个生命周期足够长的字符串。
    static std::string staticSrcDir = resources;
    
    LOG_INFO() << "srcDir: " << staticSrcDir;
    HttpConn::userCount = 0;
    HttpConn::srcDir = staticSrcDir.c_str();
}

/**
 * @brief 析构函数
 */
HttpServer::~HttpServer() {
    
}

/**
 * @brief 处理客户端连接
 * @param[in] client 客户端Socket
 */
void HttpServer::handleClient(Socket::ptr client) {
    LOG_INFO() << "handleClient " << client->getSocket();
    // 简单实现：读取数据，处理请求，发送响应
    // 实际项目中这里应该是状态机循环
    int client_socket = client->getSocket();
    if(!client->isValid()) {
        client->close();
        return;
    }

    // 确保HttpConn已初始化（通常在accept时初始化）
    // 这里假设users_已经在startAccept中被正确初始化
    
    int errnoNum = 0;
    // 1. 读取请求
    ssize_t readLen = users_[client_socket].read(&errnoNum);
    if(readLen <= 0 && errnoNum != EAGAIN) {
        LOG_ERROR() << "read error, close client: " << client_socket;
        users_[client_socket].Close();
        return;
    }

    // 2. 处理请求
    if(users_[client_socket].process()) {
        // 3. 准备响应
        // process() 返回 true 表示有数据要写
        // 4. 发送响应
        users_[client_socket].write(&errnoNum);
    }
    
    // 短连接或处理完成后关闭
    // 如果是长连接，可能需要重新注册读事件
    // 目前简单处理：一次请求后关闭
    // client->close();
}

/**
 * @brief 开始接受连接
 * @param[in] sock 服务器Socket
 */
void HttpServer::startAccept(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            // 初始化 HttpConn
            int client_socket = client->getSocket();
            sockaddr_in *addr = (sockaddr_in *)(client->getRemoteAddress()->getAddr());
            users_[client_socket].init(client_socket, *addr);
            
            m_ioWorker->schedule(std::bind(&HttpServer::handleClient, std::dynamic_pointer_cast<HttpServer>(shared_from_this()), client));
        } else {
            LOG_ERROR() << "accept errno=" << errno << " errstr=" << strerror(errno);
        }
    }
}
