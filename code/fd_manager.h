/**
 * @file fd_manager.h
 * @brief 文件句柄管理类
 * @author zch
 * @date 2025-03-30
 */

#ifndef FD_MANAGER_H__
#define FD_MANAGER_H__

#include <memory>
#include <vector>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include "./coroutine/thread.h"
#include "coroutine/singleton.h"

/**
 * @brief 文件句柄上下文
 * @details 是否 socket，是否阻塞，是否关闭，读/写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;

    FdCtx(int fd);
    ~FdCtx();
    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }
    //v 表示是否阻塞
    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getUserNonblock() const { return m_userNonblock; }
    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() const { return m_sysNonblock; }
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

private:
    bool init();

private:
    bool m_isInit : 1;
    bool m_isSocket : 1;
    //是否 hook 非阻塞
    bool m_sysNonblock : 1;
    //是否用户主动设置非阻塞
    bool m_userNonblock : 1;
    bool m_isClosed : 1;
    int m_fd;
    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};

/**
 * @brief 文件句柄管理类
 */
class FdManager {
public:
    typedef RWMutex RWMutexType;

    FdManager();
    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

#endif
