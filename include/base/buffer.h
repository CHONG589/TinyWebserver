/**
 * @file buffer.h
 * @brief 缓冲区封装
 * @author zch
 * @date 2025-03-29
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

/**
 * @attention:读写接口有两种，一个是与客户端直接IO交互所需要的读写接口，
 *            第二个是缓冲区收到了HTTP请求后，我们在处理过程中需要对缓
 *            冲区的读写接口。
 */

//可以参考这里对Buffer的讲解https://blog.csdn.net/T_Solotov/article/details/124044175

/**
 * @brief 缓冲区类
 */
class Buffer {
public:
    /**
     * @brief 构造函数
     * @param[in] initBuffSize 初始缓冲区大小
     */
    Buffer(int initBuffSize = 1024);

    /**
     * @brief 析构函数
     */
    ~Buffer() = default;

    /**
     * @brief 还可以写的数量（字节），即还剩多少空间
     * @return size_t 可写字节数
     */
    size_t WritableBytes() const;   

    /**
     * @brief 还可以读的数量，即还有多少数据在缓存
     * @return size_t 可读字节数
     */
    size_t ReadableBytes() const ;

    /**
     * @brief Buffer对外表现出queue的特性，随着读写数据，前面会空出越来越多的空间，
     *        这个空间就是Prependable空间
     * @return size_t 可预留空间字节数
     */
    size_t PrependableBytes() const;

    /**
     * @brief 返回可读数据的起始地址
     * @return const char* 可读数据起始地址
     */
    const char *Peek() const;

    /**
     * @brief 确保可写的长度
     * @param[in] len 需要确保的长度
     */
    void EnsureWriteable(size_t len);

    /**
     * @brief 移动写下标，在Append中使用
     * @param[in] len 移动的长度
     */
    void HasWritten(size_t len);

    /**
     * @brief 读取len长度的数据，移动读下标，这里读取数据只需要移动下标即可，
     *        因为读数据只要根据下标索引就行，最后再用这个函数移动读取完的数
     *        据下标
     * @param[in] len 读取的长度
     */
    void Retrieve(size_t len);

    /**
     * @brief 读取到end位置，如果向要读取len大小的数据，而buffer中没那么多，
     *        那么就只读取buffer中的所有数据就可以了。
     * @param[in] end 读取到的位置
     */
    void RetrieveUntil(const char* end);

    /**
     * @brief 读写下标归零,在别的函数中会用到
     */
    void RetrieveAll();

    /**
     * @brief 注意，这里才是真正的读取数据，读取完数据后才用Retrieve,
     *        RetrieveUntil,RetrieveAll,HasWritten这些函数移动下标。
     * @return std::string 读取到的字符串
     */
    std::string RetrieveAllToStr();

    /**
     * @brief 写指针的位置(const)
     * @return const char* 写指针位置
     */
    const char *BeginWriteConst() const;

    /**
     * @brief 写指针的位置
     * @return char* 写指针位置
     */
    char* BeginWrite();

    /**
     * @brief 添加str到缓冲区
     * @param[in] str 字符串
     */
    void Append(const std::string& str);

    /**
     * @brief 添加str到缓冲区
     * @param[in] str 字符串指针
     * @param[in] len 字符串长度
     */
    void Append(const char* str, size_t len);

    /**
     * @brief 添加data到缓冲区
     * @param[in] data 数据指针
     * @param[in] len 数据长度
     */
    void Append(const void* data, size_t len);

    /**
     * @brief 添加buff到缓冲区
     * @param[in] buff 另一个缓冲区
     */
    void Append(const Buffer& buff);

    /**
     * @brief 与客户端直接IO的读写接口（httpconn中就是调用的该接口）
     * @param[in] fd 文件描述符
     * @param[out] Errno 错误码
     * @return ssize_t 读取的字节数
     */
    ssize_t ReadFd(int fd, int* Errno);

    /**
     * @brief 与客户端直接IO的读写接口（httpconn中就是调用的该接口）
     * @param[in] fd 文件描述符
     * @param[out] Errno 错误码
     * @return ssize_t 写入的字节数
     */
    ssize_t WriteFd(int fd, int* Errno);

private:
    /**
     * @brief buffer开头指针
     * @return char* buffer开头指针
     */
    char* BeginPtr_();

    /**
     * @brief buffer开头指针(const)
     * @return const char* buffer开头指针
     */
    const char* BeginPtr_() const;

    /**
     * @brief vector不够空间是扩容用的
     * @param[in] len 需要扩容的长度
     */
    void MakeSpace_(size_t len);

    //存储数据
    std::vector<char> buffer_;  
    // 读的下标
    std::atomic<std::size_t> readPos_;  
    // 写的下标
    std::atomic<std::size_t> writePos_; 
};

#endif //BUFFER_H
