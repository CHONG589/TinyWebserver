#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

/**
 * 连接池
 */
class SqlConnPool {
public:
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, uint16_t port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool() = default;
    ~SqlConnPool() { ClosePool(); }

    int MAX_CONN_;

    // 连接池里的元素类型是 MYSQL
    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};

// 资源管理器
// 资源管理器的作用是管理资源的生命周期，在资源管理器的构造函数中申请资源，在析构函数中释放资源。
// 资源管理器的实现方式有两种：
// 1. 栈对象：资源管理器作为函数的局部变量，在函数执行结束时自动释放资源。
// 2. 堆对象：资源管理器作为动态申请的堆内存，在对象析构时自动释放资源。
// 资源管理器的优点是简化了资源的申请和释放，提高了代码的可读性和可维护性。
// 资源管理器的缺点是增加了程序的复杂度，需要在每个函数中添加资源管理器，并且需要注意资源的生命周期。

// 使用for循环来获取连接
// 这种方式不推荐，因为会导致连接泄漏，连接不会被释放，导致连接池中的连接数不断增加。
// for(int i = 0; i < 100; i++) {
//     MYSQL *conn = SqlConnPool::Instance()->GetConn();
//     // 使用连接
//     // ...
//     SqlConnPool::Instance()->FreeConn(conn);
// }

// 使用RAII机制来获取连接
// RAII（Resource Acquisition Is Initialization）即资源获取即初始化，是一种C++编程技术，
// 通过在构造函数中申请资源，在析构函数中释放资源，来保证资源的安全释放。
// 这种方式可以保证连接的安全释放，不会出现连接泄漏。
// 示例如下：
// SqlConnRAII connRAII(&sql, SqlConnPool::Instance());
// // 使用连接
// // ...
// // 连接会在connRAII对象被析构时自动释放
// // 注意：sql指针必须在函数内声明为指针类型，否则会导致连接泄漏。

// 比如上面的第一种方法，你使用要自己调用获取连接，然后使用完后自己释放，这样容易导致忘记释放连接，导致连接泄漏。
// 第二种方法，使用RAII机制，在你使用构造函数时就创建了连接连接会在connRAII对象被析构时自动释放，这样就保证了
// 连接的安全释放，不会出现连接泄漏。而且，使用RAII机制，可以保证连接的唯一性，不会出现多个线程同时获取同一个连接。
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }
    
    ~SqlConnRAII() {
        if(sql_) { connpool_->FreeConn(sql_); }
    }
    
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

#endif // SQLCONNPOOL_H
