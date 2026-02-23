#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "base/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "zchlog.h"
// #include "db/sqlconnpool.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    
    /**
     * @brief 初始化 HttpConn 对象
     * @param[in] sockFd 套接字文件描述符
     * @param[in] addr 客户端地址信息
     * @param[in] isKeepAlive 是否保持连接（服务器配置）
     */
    void init(int sockFd, const sockaddr_in& addr, bool isKeepAlive = true);

    /**
     * @brief 从客户端中读取数据，即从套接字读取
     * @param[in] saveErrno 错误码
     * @return ssize_t 读取的字节数
     */
    ssize_t read(int* saveErrno);

    /**
     * @brief 制作好的响应报文写入客户端，即写进套接字
     * @param[in] saveErrno 错误码
     * @return ssize_t 写入的字节数
     */
    ssize_t write(int* saveErrno);

    /**
     * @brief 关闭连接
     */
    void Close();

    /**
     * @brief 获取文件描述符
     * @return 文件描述符
     */
    int GetFd() const;

    /**
     * @brief 获取端口号
     * @return 端口号
     */
    int GetPort() const;

    /**
     * @brief 获取 IP 地址
     * @return const char* IP 地址
     */
    const char *GetIP() const;

    /**
     * @brief 获取客户端地址信息
     * @return sockaddr_in 客户端地址信息
     */
    sockaddr_in GetAddr() const;
    
    /**
     * @brief 处理 HTTP 请求
     * @return bool 处理是否成功
     */
    bool process();

    /**
     * @brief 获取待写入的总长度
     * @return int 待写入的总长度
     */
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    /**
     * @brief 判断是否保持连接
     * @return bool 是否保持连接
     */
    bool IsKeepAlive() const {
        return isServerKeepAlive_ && request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;  // 原子，支持锁
    
private:
   
    // 这个是 sockfd，即从 accept 返回的新连接，每个 http 对象都有自己的 fd
    int fd_;
    // 客户端地址信息
    struct sockaddr_in addr_;

    // 是否关闭连接
    bool isClose_;

    // 服务器配置的是否保持连接
    bool isServerKeepAlive_;
    // 用于 writev 系统调用的 iovec 数组
    int iovCnt_;
    struct iovec iov_[2];
    
    // 下面两个缓冲区是和 client 端交互的。
    // 存从浏览器发来的数据，要解析的请求报文就从这个缓冲区中读取
    Buffer readBuff_;
    // 将弄好的响应的报文就是写入这个缓冲区中，用来发送个浏览器
    Buffer writeBuff_;
   
    // HTTP 请求对象
    HttpRequest request_;
    // HTTP 响应对象
    HttpResponse response_;
};


#endif //HTTP_CONN_H
