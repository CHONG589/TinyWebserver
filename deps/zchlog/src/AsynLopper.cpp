#include "AsynLopper.h"

// 向生产者缓冲区放入数据
void zch::AsynLopper::Push(const char* data, size_t len) {
	{
		// 1.先对生产者缓冲区进行加锁
		std::unique_lock<std::mutex> ulk(mtxProBuf_);
		// 2.进行条件判断，如果生产者缓冲区空间足够则进行写入，否则就阻塞在生产者条件变量上面
		if (type_ == ASYNCTYPE::ASYNC_SAFE) {
            // 调用 condPro_.wait 方法，传入锁 ulk 和一个 lambda 表达式作为条件谓词，
            // 线程会在 proBuf_.WriteableSize() >= len 这个条件为 false 时被阻塞，直到
            // 有其他线程调用了 condPro_.notify_one() 或 condPro_.notify_all() 方法，
            // 并且条件 proBuf_.WriteableSize() >= len 变为 true 时，线程才会被唤醒并继
            // 续执行后续的操作。
			condPro_.wait(ulk, [&]() {return proBuf_.WriteableSize() >= len;});
		}
		// 3.条件满足，进行数据写入
		proBuf_.Push(data, len);
	}
	// 4.写入完毕，通知消费者进行数据处理
	condCon_.notify_one();
}

// 异步线程的入口函数
void zch::AsynLopper::ThreadEntry() {
	while (true) {
		{
			// 1. 判断生产者缓冲区是否有数据，有则进行交换，无则在消费者者条件变量上面进行等待
			std::unique_lock<std::mutex> ulk(mtxProBuf_);
			// 如果 _stop 为真，也可以进行向下运行，为了保证数据能够写入完毕以后再进行退出
			condCon_.wait(ulk, [&]() {return stop_ || proBuf_.ReadableSize() > 0;});
			// 退出标志被设置且生产者缓冲区没有数据，才可以退出
			if (stop_ && proBuf_.Empty()) {
				break;
			}
			proBuf_.swap(conBuf_);
		}
		// 2. 通知生产者进行数据写入
		if (type_ == ASYNCTYPE::ASYNC_SAFE) {
			condPro_.notify_all();
		}
		// 3. 消费者开始进行数据处理
		callBack_(conBuf_);
		// 4. 数据处理完毕，重新初始化消费缓冲区
		conBuf_.reset();
	}
}
