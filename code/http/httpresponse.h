#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "zchlog.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    /**
     * @brief 初始化 HttpResponse 对象
     * @param[in] srcDir 资源目录
     * @param[in] path 请求路径
     * @param[in] isKeepAlive 是否保持连接
     * @param[in] code 状态码
     */
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);

    /**
     * @brief 生成响应报文
     * @param[in] buff 写入缓冲区
     */
    void MakeResponse(Buffer& buff);

    /**
     * @brief 解除文件映射
     */
    void UnmapFile();

    /**
     * @brief 获取映射内存的起始地址
     * @return char* 映射内存的起始地址
     */
    char* File();

    /**
     * @brief 获取文件长度
     * @return size_t 文件长度
     */
    size_t FileLen() const;

    /**
     * @brief 生成错误内容
     * @param[in] buff 写入缓冲区
     * @param[in] message 错误信息
     */
    void ErrorContent(Buffer& buff, std::string message);

    /**
     * @brief 获取状态码
     * @return int 状态码
     */
    int Code() const { return code_; }

private:
    /**
     * @brief 添加状态行
     * @param[in] buff 写入缓冲区
     */
    void AddStateLine_(Buffer &buff);

    /**
     * @brief 添加响应头
     * @param[in] buff 写入缓冲区
     */
    void AddHeader_(Buffer &buff);

    /**
     * @brief 添加响应内容
     * @param[in] buff 写入缓冲区
     */
    void AddContent_(Buffer &buff);

    /**
     * @brief 处理错误 HTML
     */
    void ErrorHtml_();

    /**
     * @brief 获取文件类型
     * @return std::string 文件类型
     */
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;

    //从解析请求的那里可以知道path_中存的是如：/index.html
    std::string path_;
    std::string srcDir_;
    
    //映射区的内存起始地址
    char* mmFile_;
    //存储文件的属性
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  // 后缀类型集
    static const std::unordered_map<int, std::string> CODE_STATUS;          // 编码状态集
    static const std::unordered_map<int, std::string> CODE_PATH;            // 编码路径集
};

#endif //HTTP_RESPONSE_H
