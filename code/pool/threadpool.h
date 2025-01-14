#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <assert.h>

class ThreadPool {
public:
    ThreadPool() = default; // 默认构造函数
    ThreadPool(ThreadPool&&) = default;// 移动构造函数
    // 尽量用make_shared代替new，如果通过new再传递给shared_ptr，内存是不连续的，会造成内存碎片化
    // make_shared:传递右值，功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
    explicit ThreadPool(int threadCount = 8) 
        : pool_(std::make_shared<Pool>()) { 
        assert(threadCount > 0);
        for(int i = 0; i < threadCount; i++) {
            // 线程池里的线程都要detach，不用等待线程结束。
            // 这里用了lambda表达式，线程池里的线程都要执行一个任务，
            // 所以用了std::function<void()>作为任务类型
            std::thread([this]() {
                std::unique_lock<std::mutex> locker(pool_->mtx_);
                while(true) {
                    if(!pool_->tasks.empty()) {
                        auto task = std::move(pool_->tasks.front());    // 左值变右值,资产转移
                        pool_->tasks.pop();
                        locker.unlock();    // 因为已经把任务取出来了，所以可以提前解锁了
                        task();
                        locker.lock();      // 马上又要取任务了，上锁
                    } 
                    else if(pool_->isClosed) {
                        break;
                    } 
                    else {
                        // 任务队列为空，线程等待
                        pool_->cond_.wait(locker);    // 等待,如果任务来了就notify的
                    }
                    
                }
            }).detach();// 线程detach，不用等待线程结束。
        }
    }

    ~ThreadPool() {
        if(pool_) {
            std::unique_lock<std::mutex> locker(pool_->mtx_);
            pool_->isClosed = true;
        }
        // 因为可能有些线程因为任务队列为空，而阻塞在pool_->cond_.wait()中，
        // 此时要唤醒全部。
        pool_->cond_.notify_all();  // 唤醒所有的线程
    }

    template<typename T>
    void AddTask(T&& task) {
        std::unique_lock<std::mutex> locker(pool_->mtx_);
        pool_->tasks.emplace(std::forward<T>(task));
        pool_->cond_.notify_one();
    }

private:
    // 用一个结构体封装起来，方便调用
    struct Pool {
        std::mutex mtx_;
        std::condition_variable cond_;
        bool isClosed;
        std::queue<std::function<void()>> tasks; // 任务队列，函数类型为void()
    };
    std::shared_ptr<Pool> pool_;
};

#endif
