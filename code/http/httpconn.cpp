#include "httpconn.h"
using namespace std;

//关于类的静态变量为什么要类外初始化的文章
//https://www.cnblogs.com/lixuejian/p/13215271.html
const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    Close(); 
};

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const {
    return fd_;
};

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

ssize_t HttpConn::read(int* saveErrno) {
    LOG_INFO("start Read");
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET); // ET:边沿触发要一次性全部读出
    LOG_INFO("Read %d bytes from client", len);
    return len;
}

// 主要采用writev连续写函数
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    int remainLen;
    do {
        // 将iov的内容写到fd中
        LOG_INFO("before write");
        len = writev(fd_, iov_, iovCnt_);   
        LOG_INFO("Write %d bytes", len);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { 
            /* 传输结束 */
            break; 
        } 
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            //从iov_中写到fd的的内容大小多余iov[0]中存的，iov[0]中的内容已经写完。
            //下一次循环要从iov[1]中的len-iov[0].iov_len开始写。
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            //iov[0]中的内容已经全部写进fd_中，所以要下标清零。
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            //因为写的内容大小都还没有iov[0]中存的多，所以只需要移动iov[0]的
            //下标即可
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
        remainLen = iov_[0].iov_len + iov_[1].iov_len;

    } while(remainLen > 0);
    return len;
}

// 
bool HttpConn::process() {
    request_.Init();
    if(readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    else if(request_.parse(readBuff_)) {    // 解析成功
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        //解析失败
        response_.Init(srcDir, request_.path(), false, 400);
    }

    // 生成响应报文放入writeBuff_中
    response_.MakeResponse(writeBuff_);
    LOG_INFO("response len: %d", writeBuff_.ReadableBytes()); 
    // 响应头
    // 将写缓冲区中的内容放入类中的iov中。
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    // 文件
    if(response_.FileLen() > 0  && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_INFO("filesize:%d", iov_[0].iov_len + iov_[1].iov_len);
    return true;
}
