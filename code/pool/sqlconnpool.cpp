#include "sqlconnpool.h"

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool pool;
    return &pool;
}

// 初始化
void SqlConnPool::Init(const char* host, uint16_t port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize = 10) {
    assert(connSize > 0);
    for(int i = 0; i < connSize; i++) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if(!conn) {
            LOG_ERROR("MySql init error!");
            assert(conn);
        }
        // 获取连接
        conn = mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0);
        if (!conn) {
            LOG_ERROR("MySql Connect error!");
        }
        connQue_.emplace(conn);
    }
    MAX_CONN_ = connSize;
    // 初始化信号量，MAX_CONN 为信号量的上限。
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL *SqlConnPool::GetConn() {
    MYSQL *conn = nullptr;
    if(connQue_.empty()) {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    // 拿了一个后要减一，否则会一直阻塞
    sem_wait(&semId_);  // -1
    lock_guard<mutex> locker(mtx_);
    // 取出一个连接
    conn = connQue_.front();
    connQue_.pop();
    return conn;
}

// 释放连接，放回连接池，实际上没有关闭
void SqlConnPool::FreeConn(MYSQL* conn) {
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);  // +1
}

void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    while(!connQue_.empty()) {
        auto conn = connQue_.front();
        connQue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

// 获取当前连接池中空闲的连接数
int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}
