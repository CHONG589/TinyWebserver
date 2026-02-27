#include "http/httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css"},
    { ".js",    "text/javascript"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
};

HttpResponse::~HttpResponse() {
    UnmapFile();
}

/**
 * @brief 初始化 HttpResponse 对象
 * @param[in] srcDir 资源目录
 * @param[in] path 请求路径
 * @param[in] isKeepAlive 是否保持连接
 * @param[in] code 状态码
 */
void HttpResponse::Init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code){
    assert(srcDir != "");
    if(mmFile_) { UnmapFile(); }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
}

/**
 * @brief 生成响应报文
 * @param[in] buff 写入缓冲区
 */
void HttpResponse::MakeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    LOG_DEBUG() << "file path " << (srcDir_ + path_);
    int ret = stat(((srcDir_ + path_).data()), &mmFileStat_);
    int flag = S_ISDIR(mmFileStat_.st_mode);
    LOG_DEBUG() << "flag = " << flag << ", ret = " << ret;
    if(ret < 0 || flag) {
        code_ = 404;
    }

    //S_IROTH：有无读权限
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    } else if(code_ == -1) { 
        code_ = 200; 
    }
    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

/**
 * @brief 获取映射内存的起始地址
 * @return char* 映射内存的起始地址
 */
char* HttpResponse::File() {
    return mmFile_;
}

/**
 * @brief 获取文件长度
 * @return size_t 文件长度
 */
size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

/**
 * @brief 处理错误 HTML
 */
void HttpResponse::ErrorHtml_() {
    //.count的用法：https://blog.csdn.net/weixin_64632836/article/details/127744751
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

/**
 * @brief 添加状态行
 * @param[in] buff 写入缓冲区
 */
void HttpResponse::AddStateLine_(Buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

/**
 * @brief 添加响应头
 * @param[in] buff 写入缓冲区
 */
void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

/**
 * @brief 添加响应内容
 * @param[in] buff 写入缓冲区
 */
void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        ErrorContent(buff, "File NotFound!");
        return; 
    }

    if(mmFileStat_.st_size == 0) {
        mmFile_ = nullptr;
        close(srcFd);
        buff.Append("Content-length: 0\r\n\r\n");
        return;
    }

    // 将文件映射到内存提高文件的访问速度  MAP_PRIVATE 建立一个写入时拷贝的私有映射
    LOG_DEBUG() << "file path " << (srcDir_ + path_);
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(mmRet == (int*)MAP_FAILED) {
        LOG_ERROR() << "mmap error: " << strerror(errno) << " path=" << (srcDir_ + path_);
        ErrorContent(buff, "File NotFound!");
        return; 
    }
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

/**
 * @brief 解除文件映射
 */
void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

/**
 * @brief 获取文件类型
 * @return std::string 文件类型
 */
std::string HttpResponse::GetFileType_() {
    std::string::size_type idx = path_.find_last_of('.');
    if(idx == std::string::npos) {   // 最大值 find函数在找不到指定值得情况下会返回string::npos
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

/**
 * @brief 生成错误内容
 * @param[in] buff 写入缓冲区
 * @param[in] message 错误信息
 */
void HttpResponse::ErrorContent(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
