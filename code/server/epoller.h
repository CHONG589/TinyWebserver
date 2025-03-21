#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);
    int Wait(int timeoutMs = -1);
    //获取下标为i的事件对应的fd
    int GetEventFd(size_t i) const;
    //获取下标为i对应的事件
    uint32_t GetEvents(size_t i) const;
        
private:
    int epollFd_;
    std::vector<struct epoll_event> events_;    
};

#endif //EPOLLER_H
