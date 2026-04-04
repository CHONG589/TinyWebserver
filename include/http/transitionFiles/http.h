/**
 * @file http.h
 * @brief HTTP 定义结构体封装
 * @date 2026-03-29
 */

#ifndef HTTP_H__
#define HTTP_H__

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>

#include "http-parser/http_parser.h"

namespace zch {

namespace http {

/**
 * @brief HTTP方法枚举
 */
enum class HttpMethod {
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

/**
 * @brief HTTP状态枚举
 */
enum class HttpStatus {
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief 将字符串方法名转成HTTP方法枚举
 * @param[in] m HTTP方法
 * @return HTTP方法枚举
 */
HttpMethod StringToHttpMethod(const std::string& m);

/**
 * @brief 将字符串指针转换成HTTP方法枚举
 * @param[in] m 字符串方法枚举
 * @return HTTP方法枚举
 */
HttpMethod CharsToHttpMethod(const char* m);

/**
 * @brief 将HTTP方法枚举转换成字符串
 * @param[in] m HTTP方法枚举
 * @return 字符串
 */
const char* HttpMethodToString(const HttpMethod& m);

/**
 * @brief 将HTTP状态枚举转换成字符串
 * @param[in] m HTTP状态枚举
 * @return 字符串
 */
const char* HttpStatusToString(const HttpStatus& s);

/**
 * @brief 忽略大小写比较仿函数
 */
struct CaseInsensitiveLess {
    /**
     * @brief 忽略大小写比较字符串
     */
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

class HttpResponse;

/**
 * @brief HTTP 请求结构
 */
class HttpRequest {
public:
    typedef std::shared_ptr<HttpRequest> ptr;
    // MAP 结构
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief 构造函数
     * @param[in] version 版本
     * @param[in] close 是否 keepalive
     */
    HttpRequest(uint8_t version = 0x11, bool close = true);

    /**
     * @brief 从 HTTP 请求构造 HTTP 响应
     * @note 只需要保证请求与响应的版本号与 keep-alive 一致即可
     */
    std::shared_ptr<HttpResponse> CreateResponse();

    /**
     * @brief 返回 HTTP 方法
     */
    HttpMethod GetMethod() const { return m_method;}

    /**
     * @brief 返回HTTP版本
     */
    uint8_t GetVersion() const { return m_version;}

    /**
     * @brief 返回 HTTP 请求的路径
     */
    const std::string& GetPath() const { return m_path;}

    /**
     * @brief 返回HTTP请求的查询参数
     */
    const std::string& GetQuery() const { return m_query;}

    /**
     * @brief 返回HTTP请求的消息体
     */
    const std::string& GetBody() const { return m_body;}

    /**
     * @brief 返回 HTTP 请求的消息头 MAP
     */
    const MapType& GetHeaders() const { return m_headers;}

    /**
     * @brief 返回HTTP请求的参数MAP
     */
    const MapType& GetParams() const { return m_params;}

    /**
     * @brief 返回HTTP请求的cookie MAP
     */
    const MapType& GetCookies() const { return m_cookies;}

    /**
     * @brief 设置HTTP请求的方法名
     * @param[in] v HTTP请求
     */
    void SetMethod(HttpMethod v) { m_method = v;}

    /**
     * @brief 设置HTTP请求的协议版本
     * @param[in] v 协议版本0x11, 0x10
     */
    void SetVersion(uint8_t v) { m_version = v;}

    /**
     * @brief 设置HTTP请求的路径
     * @param[in] v 请求路径
     */
    void SetPath(const std::string& v) { m_path = v;}

    /**
     * @brief 设置HTTP请求的查询参数
     * @param[in] v 查询参数
     */
    void SetQuery(const std::string& v) { m_query = v;}

    /**
     * @brief 设置HTTP请求的Fragment
     * @param[in] v fragment
     */
    void SetFragment(const std::string& v) { m_fragment = v;}

    /**
     * @brief 设置HTTP请求的消息体
     * @param[in] v 消息体
     */
    void SetBody(const std::string& v) { m_body = v;}

    /**
     * @brief 追加HTTP请求的消息体
     * @param[in] v 追加内容
     */
    void AppendBody(const std::string &v) { m_body.append(v); }

    /**
     * @brief 是否自动关闭
     */
    bool IsClose() const { return m_close;}

    /**
     * @brief 设置是否自动关闭
     */
    void SetClose(bool v) { m_close = v;}

    /**
     * @brief 是否 websocket
     */
    bool IsWebsocket() const { return m_websocket;}

    /**
     * @brief 设置是否websocket
     */
    void SetWebsocket(bool v) { m_websocket = v;}

    /**
     * @brief 设置HTTP请求的头部MAP
     * @param[in] v map
     */
    void SetHeaders(const MapType& v) { m_headers = v;}

    /**
     * @brief 设置HTTP请求的参数MAP
     * @param[in] v map
     */
    void SetParams(const MapType& v) { m_params = v;}

    /**
     * @brief 设置HTTP请求的Cookie MAP
     * @param[in] v map
     */
    void SetCookies(const MapType& v) { m_cookies = v;}

    /**
     * @brief 获取HTTP请求的头部参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string GetHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 提取url中的查询参数
     */
    void InitQueryParam();

    /**
     * @brief 当content-type是application/x-www-form-urlencoded时，提取消息体中的表单参数
     */
    void InitBodyParam();

    /**
     * @brief 获取HTTP请求的请求参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string GetParam(const std::string& key, const std::string& def = "");

    /**
     * @brief 提取请求中的cookies
     */
    void InitCookies();

    /**
     * @brief 获取HTTP请求的Cookie参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string GetCookie(const std::string& key, const std::string& def = "");

    /**
     * @brief 设置HTTP请求的头部参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void SetHeader(const std::string& key, const std::string& val);

    /**
     * @brief 设置HTTP请求的请求参数
     * @param[in] key 关键字
     * @param[in] val 值
     */

    void SetParam(const std::string& key, const std::string& val);

    /**
     * @brief 设置HTTP请求的Cookie参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void SetCookie(const std::string& key, const std::string& val);

    /**
     * @brief 删除HTTP请求的头部参数
     * @param[in] key 关键字
     */
    void DelHeader(const std::string& key);

    /**
     * @brief 删除HTTP请求的请求参数
     * @param[in] key 关键字
     */
    void DelParam(const std::string& key);

   /**
     * @brief 删除HTTP请求的Cookie参数
     * @param[in] key 关键字
     */
    void DelCookie(const std::string& key);

    /**
     * @brief 判断HTTP请求的头部参数是否存在
     * @param[in] key 关键字
     * @param[out] val 如果存在,val非空则赋值
     * @return 是否存在
     */
    bool HasHeader(const std::string& key, std::string* val = nullptr);

    /**
     * @brief 判断HTTP请求的请求参数是否存在
     * @param[in] key 关键字
     * @param[out] val 如果存在,val非空则赋值
     * @return 是否存在
     */
    bool HasParam(const std::string& key, std::string* val = nullptr);

    /**
     * @brief 判断HTTP请求的Cookie参数是否存在
     * @param[in] key 关键字
     * @param[out] val 如果存在,val非空则赋值
     * @return 是否存在
     */
    bool HasCookie(const std::string& key, std::string* val = nullptr);

private:
    // HTTP方法
    HttpMethod m_method;
    // HTTP版本
    uint8_t m_version;
    // 是否自动关闭
    bool m_close;
    // 是否为 websocket
    bool m_websocket;
    // 参数解析标志位，0:未解析，1:已解析url参数, 2:已解析http消息体中的参数，4:已解析cookies
    uint8_t m_parserParamFlag;
    // 请求的完整url
    std::string m_url;
    // 请求路径
    std::string m_path;
    // 请求参数
    std::string m_query;
    // 请求fragment
    std::string m_fragment;
    // 请求消息体
    std::string m_body;
    // 请求头部 MAP
    MapType m_headers;
    // 请求参数 MAP
    MapType m_params;
    // 请求 Cookie MAP
    MapType m_cookies;
};

}

}

#endif
