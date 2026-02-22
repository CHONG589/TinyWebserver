#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>    // 正则表达式
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../pool/sqlconnpool.h"
#include "zchlog.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    /**
     * @brief 初始化 HttpRequest 对象
     */
    void Init();

    /**
     * @brief 解析 HTTP 请求
     * @param[in] buff 读缓冲区
     * @return bool 解析是否成功
     */
    bool parse(Buffer& buff);   

    /**
     * @brief 获取请求路径
     * @return std::string 请求路径
     */
    std::string path() const;
    std::string& path();

    /**
     * @brief 获取请求方法
     * @return std::string 请求方法
     */
    std::string method() const;

    /**
     * @brief 获取 HTTP 版本
     * @return std::string HTTP 版本
     */
    std::string version() const;

    /**
     * @brief 获取 POST 请求参数
     * @param[in] key 参数名
     * @return std::string 参数值
     */
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    /**
     * @brief 判断是否保持连接
     * @return bool 是否保持连接
     */
    bool IsKeepAlive() const;

private:
    /**
     * @brief 解析请求行
     * @param[in] line 请求行字符串
     * @return bool 解析是否成功
     */
    bool ParseRequestLine_(const std::string& line);

    /**
     * @brief 解析请求头
     * @param[in] line 请求头字符串
     */
    void ParseHeader_(const std::string& line);

    /**
     * @brief 解析请求体
     * @param[in] line 请求体字符串
     */
    void ParseBody_(const std::string& line);

    /**
     * @brief 处理请求路径
     */
    void ParsePath_();

    /**
     * @brief 处理 Post 请求
     */
    void ParsePost_();

    /**
     * @brief 从 urlencoded 格式中解析参数
     */
    void ParseFromUrlencoded_();

    /**
     * @brief 用户验证
     * @param[in] name 用户名
     * @param[in] pwd 密码
     * @param[in] isLogin 是否为登录操作
     * @return bool 验证是否通过
     */
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;                                         // 解析状态
    std::string method_, path_, version_, body_;                // 请求方法，路径，版本，请求体
    std::unordered_map<std::string, std::string> header_;       // 请求头
    std::unordered_map<std::string, std::string> post_;         // POST 请求参数

    static const std::unordered_set<std::string> DEFAULT_HTML;              // 默认网页
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;     // 默认网页标签
    
    /**
     * @brief 16进制转换为10进制
     * @param[in] ch 16进制字符
     * @return int 10进制数值
     */
    static int ConverHex(char ch);
};

#endif
