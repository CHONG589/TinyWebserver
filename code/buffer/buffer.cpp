#include "buffer.h"

// 读写下标初始化，vector<char>初始化
Buffer::Buffer(int initBuffSize) 
            : buffer_(initBuffSize)
            , readPos_(0)
            , writePos_(0)  {}  

// 可写的数量：buffer大小 - 写下标
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 可读的数量：写下标 - 读下标
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 可预留空间：已经读过的就没用了，等于读下标
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char* Buffer::Peek() const {
    
    return &buffer_[readPos_];
}

// 确保可写的长度
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

// 移动写下标，在Append中使用
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

// 读取len长度，移动读下标
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// 读取到end位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek()); // end指针 - 读指针长度
}

// 取出所有数据，buffer归零，读写下标归零,在别的函数中会用到
void Buffer::RetrieveAll() {
    //将参数所指的内存区域前buffer_.size()个字节全部设为零
    bzero(&buffer_[0], buffer_.size()); // 覆盖原本数据
    readPos_ = writePos_ = 0;
}

//注意，这里才是真正的读取数据，读取完数据后才用Retrieve,
//RetrieveUntil,RetrieveAll,HasWritten这些函数移动下标。
std::string Buffer::RetrieveAllToStr() {
    //从第一个参数的位置开始读，读取第二个参数的数量大小的数据
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// 写指针的位置
const char *Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

char *Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// 添加str到缓冲区
void Buffer::Append(const char *str, size_t len) {
    assert(str);
    // 确保可写的长度
    EnsureWriteable(len);   
    //将str放到写下标开始的地方
    std::copy(str, str + len, BeginWrite());
    // 移动写下标
    HasWritten(len);    
}

void Buffer::Append(const std::string& str) {
    Append(str.c_str(), str.size());
}

void Append(const void *data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// 将buffer中的读下标的地方放到该buffer中的写下标位置
// 即将buff中的数据复制到缓冲区中，这里有可能是另一个
// 缓冲区的数据移动到这个缓冲区中。
void Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 将fd的内容读到缓冲区，即writable的位置
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

// 将buffer中可读的区域写入fd中
ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if(len < 0) {
        *Errno = errno;
        return len;
    } 
    Retrieve(len);
    return len;
}

char *Buffer::BeginPtr_() {
    return &buffer_[0];
}

const char *Buffer::BeginPtr_() const{
    return &buffer_[0];
}

// 扩展空间
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
