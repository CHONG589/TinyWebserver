/**
 * @file AsynLopper.h
 * @brief 异步工作器的实现
 * @author zch
 * @date 2025-11-09
 */

#ifndef ASYNLOPPER_H__
#define ASYNLOPPER_H__

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <memory>

#include "Buffer.hpp"

namespace zch {

    enum class ASYNCTYPE {
        ASYNC_SAFE,
        ASYNC_UN_SAFE
    };

    class AsynLopper {
    public:
        using cb_t = std::function<void(zch::Buffer&)>;
		using ptr = std::shared_ptr<zch::AsynLopper>;

        AsynLopper(cb_t callBack, ASYNCTYPE type = ASYNCTYPE::ASYNC_SAFE)
			        : type_(type) 
					, stop_(false)
                    , td_(&AsynLopper::ThreadEntry, this)
					, callBack_(callBack) {}

        // 向生产者缓冲区放入数据
		void Push(const char* data, size_t len);

        // 停止异步线程的工作
		void Stop() {
			stop_ = true;
			// 唤醒异步线程，进行退出
			condCon_.notify_all();
			// 回收异步线程
			td_.join();
		}

        ~AsynLopper() {
			// 停止异步线程
			Stop();
		}

    private:
        // 异步线程的入口函数
		void ThreadEntry();

    private:
        // 异步工作器的安全类型
		ASYNCTYPE type_;
		// 线程的工作状态 
		// (由于日志线程需要读取此变量的状态，而上层的业务线程可能会对这个变量进行修改，
		// 因此这个变量存在线程安全问题，我们这里使用原子类型)
		std::atomic<bool>  stop_;
		// 保护生产者缓冲区的锁
		std::mutex mtxProBuf_;
		// 生产者缓冲区
		zch::Buffer proBuf_;
		// 消费者缓冲区
		zch::Buffer conBuf_;
		// 生产者条件变量
		std::condition_variable condPro_;
		// 消费者条件变量
		std::condition_variable condCon_;
		// 异步线程对象
		std::thread td_;
		// 线程对象的回调函数
		cb_t callBack_;
    };
}

#endif
