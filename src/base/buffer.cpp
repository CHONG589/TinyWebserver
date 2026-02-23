#include "base/buffer.h"

/**
 * @brief 构造函数
 * @param[in] initBuffSize 初始缓冲区大小
 */
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

/**
 * @brief 还可以写的数量（字节），即还剩多少空间
 * @return size_t 可写字节数
 */
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

/**
 * @brief 还可以读的数量，即还有多少数据在缓存
 * @return size_t 可读字节数
 */
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

/**
 * @brief Buffer对外表现出queue的特性，随着读写数据，前面会空出越来越多的空间，
 *        这个空间就是Prependable空间
 * @return size_t 可预留空间字节数
 */
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

/**
 * @brief 返回可读数据的起始地址
 * @return const char* 可读数据起始地址
 */
const char* Buffer::Peek() const {
    
    return &buffer_[readPos_];
}

/**
 * @brief 确保可写的长度
 * @param[in] len 需要确保的长度
 */
void Buffer::EnsureWriteable(size_t len) {
    if(len > WritableBytes()) {
        //只要判断想要的空间len大于后面可写的空间，就用
        //MakeSpace()函数扩容，扩容也分两种情况：
        //1.如果后面的空间和前面Prependable的空间大于
        //len，那么不用实际的扩容，只需要将数据向前移一
        //下就可以。
        //2.如果加起来还不够，那么就要扩容
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

/**
 * @brief 移动写下标，在Append中使用
 * @param[in] len 移动的长度
 */
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

/**
 * @brief 读取len长度的数据，移动读下标，这里读取数据只需要移动下标即可，
 *        因为读数据只要根据下标索引就行，最后再用这个函数移动读取完的数
 *        据下标
 * @param[in] len 读取的长度
 */
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

/**
 * @brief 读取到end位置，如果向要读取len大小的数据，而buffer中没那么多，
 *        那么就只读取buffer中的所有数据就可以了。
 * @param[in] end 读取到的位置
 */
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek()); // end指针 - 读指针长度
}

/**
 * @brief 读写下标归零,在别的函数中会用到
 */
void Buffer::RetrieveAll() {
    //将参数所指的内存区域前buffer_.size()个字节全部设为零
    bzero(&buffer_[0], buffer_.size()); // 覆盖原本数据
    readPos_ = writePos_ = 0;
}

/**
 * @brief 注意，这里才是真正的读取数据，读取完数据后才用Retrieve,
 *        RetrieveUntil,RetrieveAll,HasWritten这些函数移动下标。
 * @return std::string 读取到的字符串
 */
std::string Buffer::RetrieveAllToStr() {
    //从第一个参数的位置开始读，读取第二个参数的数量大小的数据
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

/**
 * @brief 写指针的位置(const)
 * @return const char* 写指针位置
 */
const char *Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

/**
 * @brief 写指针的位置
 * @return char* 写指针位置
 */
char *Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

/**
 * @brief 添加str到缓冲区
 * @param[in] str 字符串指针
 * @param[in] len 字符串长度
 */
void Buffer::Append(const char *str, size_t len) {
    assert(str);
    // 确保可写的长度
    EnsureWriteable(len);   
    //将str放到写下标开始的地方
    std::copy(str, str + len, BeginWrite());
    // 移动写下标
    HasWritten(len);    
}

/**
 * @brief 添加str到缓冲区
 * @param[in] str 字符串
 */
void Buffer::Append(const std::string& str) {
    Append(str.c_str(), str.size());
}

/**
 * @brief 添加data到缓冲区
 * @param[in] data 数据指针
 * @param[in] len 数据长度
 */
void Buffer::Append(const void *data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

/**
 * @brief 添加buff到缓冲区
 * @param[in] buff 另一个缓冲区
 */
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

/**
 * @brief 与客户端直接IO的读写接口（httpconn中就是调用的该接口）
 * @param[in] fd 文件描述符
 * @param[out] Errno 错误码
 * @return ssize_t 读取的字节数
 */
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];   // 栈区
    struct iovec iov[2];
    size_t writeable = WritableBytes(); // 先记录能写多少
    // 分散读，保证数据全部读完
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *Errno = errno;
    } 
    else if(static_cast<size_t>(len) <= writeable) {
        // 若len小于writable，说明写区可以容纳len
        writePos_ += len;
    } 
    else {  
        // 写区写满了,下标移到最后  
        writePos_ = buffer_.size(); 
        //通过readv已经将fd中的数据读取到iov中，这里要将buff中的数据
        //读到缓冲区中。
        //Append中会有扩容操作，容量不够，会扩容，这里是先读到缓冲区iov中，
        //然后再读到Buffer中。
        Append(buff, static_cast<size_t>(len - writeable)); // 剩余的长度
    }
    return len;
}

/**
 * @brief 与客户端直接IO的读写接口（httpconn中就是调用的该接口）
 * @param[in] fd 文件描述符
 * @param[out] Errno 错误码
 * @return ssize_t 写入的字节数
 */
ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if(len < 0) {
        *Errno = errno;
        return len;
    } 
    Retrieve(len);
    return len;
}

/**
 * @brief buffer开头指针
 * @return char* buffer开头指针
 */
char *Buffer::BeginPtr_() {
    return &buffer_[0];
}

/**
 * @brief buffer开头指针(const)
 * @return const char* buffer开头指针
 */
const char *Buffer::BeginPtr_() const{
    return &buffer_[0];
}

/**
 * @brief vector不够空间是扩容用的
 * @param[in] len 需要扩容的长度
 */
void Buffer::MakeSpace_(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) {
        //这里扩容的大小是在原有数据的基础上，然后再加上len个数据。
        buffer_.resize(writePos_ + len + 1);
    } 
    else {
        size_t readable = ReadableBytes();
        //将数据往前移
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        assert(readable == ReadableBytes());
    }
}
