#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fd_manager.h"

bool FdCtx::init() {
    if(m_isInit) {
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    //返回-1 表示失败，成功返回0，将m_fd所指向的文件
    //的属性状态信息存储在fd_stat中。
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } 
    else {
        m_isInit = true;
        //判断fd_stat所指向的文件是否是socket
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if(m_isSocket) {
        // int flags = fcntl_f(m_fd, F_GETFL, 0);
        // if(!(flags & O_NONBLOCK)) {
        //     fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        // }
        // m_sysNonblock = true;
    }
    else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

FdCtx::FdCtx(int fd)
    : m_isInit(false)
    , m_isSocket(false)
    , m_sysNonblock(false)
    , m_userNonblock(false)
    , m_isClosed(false)
    , m_fd(fd)
    , m_recvTimeout(-1)
    , m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } 
    else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } 
    else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) {
        return nullptr;
    }
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        if(auto_create == false) {
            return nullptr;
        }
    } 
    else {
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();

    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return ;
    }
    m_datas[fd].reset();
}
