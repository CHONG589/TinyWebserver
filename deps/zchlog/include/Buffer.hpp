/**
 * @file Buffer.hpp
 * @brief 异步线程缓冲区的设计
 * @author zch
 * @date 2025-11-09
 */

#ifndef BUFFER_H__
#define BUFFER_H__

#include <iostream>
#include <vector>
#include <assert.h>

namespace zch {
    // 缓冲区默认大小
	const size_t defaultBufferSize = 1 * 1024 * 1024;
	// 阈值
	const size_t threshold = 2 * 1024 * 1024;
	// 大于阈值以后每次扩容的自增值
	const size_t increament = 1 * 1024 * 1024;

    class Buffer {
    public:
        /** 
         * @brief 构造函数，初始化缓冲区大小
         * @param[in] bufferSize 缓冲区初始大小，默认 1MB
         */
        Buffer(size_t bufferSize = defaultBufferSize) 
				: buffer_(bufferSize)
				, readIdx_(0)
				, writeIdx_(0) {}

        /** 
         * @brief 获取当前可写入的空间大小
         * @return size_t 可写字节数
         */
		size_t WriteableSize() { return buffer_.size() - writeIdx_; }

        /** 
         * @brief 获取当前可读取的数据大小
         * @return size_t 可读字节数
         */
		size_t ReadableSize() { return writeIdx_ - readIdx_; }

        /** 
         * @brief 向后移动读指针（表示数据已被读取）
         * @param[in] len 移动的长度
         * @return void
         */
		void MoveReadIdx(size_t len) {
			// 防止外界传入的参数不合法
			if (len >= ReadableSize()) {
				readIdx_ = writeIdx_;
			} else {
				readIdx_ += len;
			}
		}

        /** 
         * @brief 获取可读数据的起始地址
         * @return const char* 数据起始指针
         */
		const char* Start() { return &buffer_[readIdx_]; }

        /** 
         * @brief 重置缓冲区（清空读写指针）
         * @return void
         */
		void reset() {
			writeIdx_ = 0; 
            readIdx_ = 0;
		}

        /** 
         * @brief 交换两个缓冲区的内容（O(1) 复杂度）
         * @param[in] buf 要交换的目标缓冲区
         * @return void
         */
		void swap(Buffer& buf) {
            // 首先交换读写指针
			std::swap(writeIdx_, buf.writeIdx_);
			std::swap(readIdx_, buf.readIdx_);

            // 交换两个 std::vector 对象所管理的内存区域，
            // 它是非复制的。它交换的是指向底层数据、大小和
            // 容量的内部指针，而不是复制大量的元素，所以它
            // 的时间复杂度是常数级的 O(1)。
			buffer_.swap(buf.buffer_);
		}

        /** 
         * @brief 判断缓冲区是否为空
         * @return bool 为空返回 true，否则 false
         */
		bool Empty() { return writeIdx_ == readIdx_; }
        
        /** 
         * @brief 向缓冲区推入数据
         * @param[in] data 数据源指针
         * @param[in] len 数据长度
         * @return void
         */
		inline void Push(const char* data, size_t len) {
			// 1. 判断空间是否足够
			if (len > WriteableSize()) {
                Resize(len);
            }

			// 2. 将数据写入缓冲区
			std::copy(data, data + len, &buffer_[writeIdx_]);

			// 3. 更新数据的写入位置
			MoveWriteIdx(len);
		}

    private:
        /** 
         * @brief 向后移动写指针（内部调用）
         * @param[in] len 移动的长度
         * @return void
         */
		void MoveWriteIdx(size_t len) {
			assert(writeIdx_ + len <= buffer_.size());
			writeIdx_ += len;
		}

        /** 
         * @brief 扩容缓冲区
         * @param[in] len 需要增加的最小长度
         * @return void
         */
		void Resize(size_t len) {
			size_t newSize;
			if (buffer_.size() < threshold) {
				newSize = buffer_.size() * 2 + len;
			} else {
				newSize = buffer_.size() + increament + len;
			}
			buffer_.resize(newSize);
		}
        
    private:
        // 日志缓存区
		std::vector<char> buffer_;
		// 可读位置的起始下标
		size_t readIdx_;
		// 可写位置的起始下标
		size_t writeIdx_;
    };
}

#endif
