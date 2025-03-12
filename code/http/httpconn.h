#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
/*
进行读写数据并调用httprequest 来解析数据以及httpresponse来生成响应
*/
class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    
    void init(int sockFd, const sockaddr_in& addr);
    //从客户端中读取数据，即从套接字读取
    ssize_t read(int* saveErrno);
    //制作好的响应报文写入客户端，即写进套接字
    ssize_t write(int* saveErrno);
    void Close();
    int GetFd() const;
    int GetPort() const;
    const char *GetIP() const;
    sockaddr_in GetAddr() const;
    bool process();

    // 写的总长度
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;  // 原子，支持锁
    
private:
   
    //这个是 sockfd，即从 accept 返回的新连接，每个
    //http 对象都有自己的 fd
    int fd_;
    struct  sockaddr_in addr_;

    bool isClose_;
    
    int iovCnt_;
    struct iovec iov_[2];
    
    //下面两个缓冲区是和 client 端交互的。
    //存从浏览器发来的数据，要解析的请求报文就从这个缓冲区中读取
    Buffer readBuff_;
    //将弄好的响应的报文就是写入这个缓冲区中，用来发送个浏览器
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;
};


#endif //HTTP_CONN_H
