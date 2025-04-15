#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "../code/coroutine/iomanager.h"

int sockfd;
void watch_io_read();

// 写事件回调，只执行一次，用于判断非阻塞套接字connect成功
void do_io_write() {
    LOG_INFO("do_io_write");
    int so_err;
    socklen_t len = size_t(so_err);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_err, &len);
    if(so_err) {
        LOG_INFO("connect fail, so_err=%d", so_err);
        return;
    } 
    LOG_INFO("connect success");
}

// 读事件回调，每次读取之后如果套接字未关闭，需要重新添加
void do_io_read() {
    LOG_INFO("do_io_read");
    char buf[1024] = {0};
    int readlen = 0;
    readlen = read(sockfd, buf, sizeof(buf));
    if(readlen > 0) {
        buf[readlen] = '\0';
        LOG_INFO("read %d bytes, read: %s", readlen, buf);
    } else if(readlen == 0) {
        LOG_INFO("peer closed");
        close(sockfd);
        return;
    } else {
        LOG_INFO("read error, errno=%d, errstr=%s", errno, strerror(errno));
        close(sockfd);
        return;
    }
    // read之后重新添加读事件回调，这里不能直接调用addEvent，因为在当前位置fd的读事件上下文还是有效的，直接调用addEvent相当于重复添加相同事件
    IOManager::GetThis()->schedule(watch_io_read);
}

void watch_io_read() {
    LOG_INFO("start read");
    IOManager::GetThis()->addEvent(sockfd, IOManager::READ, do_io_read);
}

void start_accept() {
    LOG_INFO("start accept");
}

void test_io() {

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(6656);
    inet_pton(AF_INET, "127.0.0.1", &laddr.sin_addr);

    if(bind(listenfd, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if(listen(listenfd, 5) < 0) {
        perror("listen");
        exit(1);
    }
    IOManager::GetThis()->addEvent(sockfd, IOManager::READ, do_io_read);

    // sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // assert(sockfd > 0);
    // fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // sockaddr_in servaddr;
    // memset(&servaddr, 0, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_port = htons(1234);
    // inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr.s_addr);

    // int rt = connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr));
    // if(rt != 0) {
    //     if(errno == EINPROGRESS) {
    //         LOG_INFO("EINPROGRESS");
    //         // 注册写事件回调，只用于判断connect是否成功
    //         // 非阻塞的TCP套接字connect一般无法立即建立连接，要通过套接字可写来判断connect是否已经成功
    //         IOManager::GetThis()->addEvent(sockfd, IOManager::WRITE, do_io_write);
    //         // 注册读事件回调，注意事件是一次性的
    //         IOManager::GetThis()->addEvent(sockfd, IOManager::READ, do_io_read);
    //     } else {
    //         LOG_INFO("connect error, errno:%d, errstr:%s", errno, strerror(errno));
    //     }
    // } else {
    //     //SYLAR_LOG_ERROR(g_logger) << "else, errno:" << errno << ", errstr:" << strerror(errno);
    // }
}

void test_iomanager() {
    IOManager iom;
    // sylar::IOManager iom(10); // 演示多线程下IO协程在不同线程之间切换
    // 添加调度任务 test_io
    LOG_INFO("add test_io");
    iom.schedule(test_io);
}

int main(int argc, char *argv[]) {

    Log::Instance()->init(1, "./log", ".log", 1024);
    
    test_iomanager();

    return 0;
}
