#include "http/httpconn.h"

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
    isServerKeepAlive_ = false;
};

HttpConn::~HttpConn() { 
    Close(); 
};

/**
* @brief 初始化 HttpConn 对象
* @param[in] sockFd 套接字文件描述符
* @param[in] addr 客户端地址信息
* @param[in] isKeepAlive 是否保持连接（服务器配置）
*/
void HttpConn::init(int fd, const sockaddr_in& addr, bool isKeepAlive) {

    if(fd <= 0) {
        LOG_WARN() << "HttpConn::init fd <= 0";
        return;
    }

    userCount++;
    addr_ = addr;
    fd_ = fd;
    isServerKeepAlive_ = isKeepAlive;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_DEBUG() << "Client[" << fd_ << "]" << " in, userCount:" << userCount;
}

/**
* @brief 关闭连接
*/
void HttpConn::Close() {
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_DEBUG() << "Client[" << fd_ << "]" << " quit, UserCount:" << userCount;
    }
}

/**
* @brief 获取文件描述符
* @return 文件描述符
*/
int HttpConn::GetFd() const {
    return fd_;
};

/**
* @brief 获取客户端地址信息
* @return sockaddr_in 客户端地址信息
*/
struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

/**
* @brief 获取 IP 地址
* @return const char* IP 地址
*/
const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

/**
* @brief 获取端口号
* @return 端口号
*/
int HttpConn::GetPort() const {
    return addr_.sin_port;
}

/**
 * @brief 从客户端中读取数据，即从套接字读取
 * @param[in] saveErrno 错误码
 * @return ssize_t 读取的字节数
 */
ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET); // ET:边沿触发要一次性全部读出

    return len;
}

/**
 * @brief 制作好的响应报文写入客户端，即写进套接字
 * @param[in] saveErrno 错误码
 * @return ssize_t 写入的字节数
 */
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    int remainLen;
    do {
        // 将iov的内容写到fd中
        len = writev(fd_, iov_, iovCnt_);   
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { 
            /* 传输结束 */
            break; 
        } else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            //从iov_中写到fd的的内容大小多余iov[0]中存的，iov[0]中的内容已经写完。
            //下一次循环要从iov[1]中的len-iov[0].iov_len开始写。
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            //iov[0]中的内容已经全部写进fd_中，所以要下标清零。
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
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

/**
 * @brief 处理 HTTP 请求
 * @return bool 处理是否成功
 */
bool HttpConn::process() {
    request_.Init();
    if(readBuff_.ReadableBytes() <= 0) {
        LOG_WARN() << "HTTP 请求中没有数据";
        return false;
    } else if(request_.parse(readBuff_)) {    // 解析成功
        LOG_INFO() << "解析 HTTP 请求成功";
        LOG_DEBUG() << request_.path();
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        //解析失败
        LOG_WARN() << "解析 HTTP 请求失败";
        response_.Init(srcDir, request_.path(), false, 400);
    }

    // 生成响应报文放入writeBuff_中
    response_.MakeResponse(writeBuff_);
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
    return true;
}
