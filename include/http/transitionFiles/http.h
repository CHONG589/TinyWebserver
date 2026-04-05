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
#include <string.h>

#include "http/http-parser/http_parser.h"

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

/**
 * @brief 类型转换模板类(F 源类型, T 目标类型)
 */
template <class F, class T>
class LexicalCast {
public: 
    /**
     * @brief 类型转换
     * @param[in] v 源类型值
     * @return 返回v转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator()(const F &v) {

        T ret;
        std::stringstream ss;
        ss << v;
        ss >> ret;
        return ret;
    }
};

/**
 * @brief 获取Map中的key值,并转成对应类型,返回是否成功
 * @param[in] m Map数据结构
 * @param[in] key 关键字
 * @param[out] val 保存转换后的值
 * @param[in] def 默认值
 * @return
 *      @retval true 转换成功, val 为对应的值
 *      @retval false 不存在或者转换失败 val = def
 */
template<class MapType, class T>
bool CheckGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        val = def;
        return false;
    }

    try {
        val = LexicalCast<MapType, T>(it->second);
        return true;
    } catch (...) {
        val = def;
    }

    return false;
}

/**
 * @brief 获取Map中的key值,并转成对应类型
 * @param[in] m Map数据结构
 * @param[in] key 关键字
 * @param[in] def 默认值
 * @return 如果存在且转换成功返回对应的值,否则返回默认值
 */
template<class MapType, class T>
T GetAs(const MapType& m, const std::string& key, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }
    try {
        return LexicalCast<MapType, T>(it->second);
    } catch (...) {
    }

    return def;
}

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

    /**
     * @brief 检查并获取HTTP请求的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[out] val 返回值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T>
    bool CheckGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return CheckGetAs(m_headers, key, val, def);
    }

    /**
     * @brief 获取HTTP请求的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T>
    T GetHeaderAs(const std::string& key, const T& def = T()) {
        return GetAs(m_headers, key, def);
    }

    /**
     * @brief 检查并获取HTTP请求的请求参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[out] val 返回值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T>
    bool CheckGetParamAs(const std::string& key, T& val, const T& def = T()) {
        InitQueryParam();
        InitBodyParam();

        return CheckGetAs(m_params, key, val, def);
    }

    /**
     * @brief 获取HTTP请求的请求参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T>
    T GetParamAs(const std::string& key, const T& def = T()) {
        InitQueryParam();
        InitBodyParam();

        return GetAs(m_params, key, def);
    }

    /**
     * @brief 检查并获取HTTP请求的Cookie参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[out] val 返回值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T>
    bool CheckGetCookieAs(const std::string& key, T& val, const T& def = T()) {
        InitCookies();

        return CheckGetAs(m_cookies, key, val, def);
    }

    /**
     * @brief 获取HTTP请求的Cookie参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T>
    T GetCookieAs(const std::string& key, const T& def = T()) {
        InitCookies();

        return GetAs(m_cookies, key, def);
    }

    /**
     * @brief 序列化输出到流中
     * @param[in, out] os 输出流
     * @return 输出流
     */
    std::ostream& Dump(std::ostream& os) const;

    /**
     * @brief 转成字符串类型
     * @return 字符串
     */
    std::string ToString() const;

    /**
     * @brief 初始化，实际是判断connection是否为keep-alive，以设置是否自动关闭套接字
     */
    void Init();

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

/**
 * @brief HTTP响应结构体
 */
class HttpResponse {
public:
    typedef std::shared_ptr<HttpResponse> ptr;

    // MapType
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief 构造函数
     * @param[in] version 版本
     * @param[in] close 是否自动关闭
     */
    HttpResponse(uint8_t version = 0x11, bool close = true);

    /**
     * @brief 返回响应状态
     * @return 请求状态
     */
    HttpStatus GetStatus() const { return m_status;}

    /**
     * @brief 返回响应版本
     * @return 版本
     */
    uint8_t GetVersion() const { return m_version;}

    /**
     * @brief 返回响应消息体
     * @return 消息体
     */
    const std::string& GetBody() const { return m_body;}

    /**
     * @brief 返回响应原因
     */
    const std::string& GetReason() const { return m_reason;}

    /**
     * @brief 返回响应头部MAP
     * @return MAP
     */
    const MapType& GetHeaders() const { return m_headers;}

    /**
     * @brief 设置响应状态
     * @param[in] v 响应状态
     */
    void SetStatus(HttpStatus v) { m_status = v;}

    /**
     * @brief 设置响应版本
     * @param[in] v 版本
     */
    void SetVersion(uint8_t v) { m_version = v;}

    /**
     * @brief 设置响应消息体
     * @param[in] v 消息体
     */
    void SetBody(const std::string& v) { m_body = v;}

    /**
     * @brief 追加HTTP请求的消息体
     * @param[in] v 追加内容
     */
    void AppendBody(const std::string &v) { m_body.append(v); }

    /**
     * @brief 设置响应原因
     * @param[in] v 原因
     */
    void SetReason(const std::string& v) { m_reason = v;}

    /**
     * @brief 设置响应头部MAP
     * @param[in] v MAP
     */
    void SetHeaders(const MapType& v) { m_headers = v;}

    /**
     * @brief 是否自动关闭
     */
    bool IsClose() const { return m_close;}

    /**
     * @brief 设置是否自动关闭
     */
    void SetClose(bool v) { m_close = v;}

    /**
     * @brief 是否websocket
     */
    bool IsWebsocket() const { return m_websocket;}

    /**
     * @brief 设置是否websocket
     */
    void SetWebsocket(bool v) { m_websocket = v;}

    /**
     * @brief 获取响应头部参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在返回对应值,否则返回def
     */
    std::string GetHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 设置响应头部参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void SetHeader(const std::string& key, const std::string& val);

    /**
     * @brief 删除响应头部参数
     * @param[in] key 关键字
     */
    void DelHeader(const std::string& key);

    /**
     * @brief 检查并获取响应头部参数
     * @tparam T 值类型
     * @param[in] key 关键字
     * @param[out] val 值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T>
    bool CheckGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return CheckGetAs(m_headers, key, val, def);
    }

    /**
     * @brief 获取响应的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T>
    T GetHeaderAs(const std::string& key, const T& def = T()) {
        return GetAs(m_headers, key, def);
    }

    /**
     * @brief 序列化输出到流
     * @param[in, out] os 输出流
     * @return 输出流
     */
    std::ostream& Dump(std::ostream& os) const;

    /**
     * @brief 转成字符串
     */
    std::string ToString() const;

    /**
     * @brief 设置重定向，在头部添加Location字段，值为uri
     * @param[] uri 目标uri
     */
    void SetRedirect(const std::string& uri);

    /**
     * @brief 为响应添加cookie
     * @param[] key cookie的key值
     * @param[] val cookie的value
     * @param[] expired 过期时间
     * @param[] path cookie的影响路径
     * @param[] domain cookie作用的域
     * @param[] secure 安全标志
     */
    void SetCookie(const std::string& key, const std::string& val,
                   time_t expired = 0, const std::string& path = "",
                   const std::string& domain = "", bool secure = false);

private:
    // 响应状态
    HttpStatus m_status;
    // 版本
    uint8_t m_version;
    // 是否自动关闭
    bool m_close;
    // 是否为websocket
    bool m_websocket;
    // 响应消息体
    std::string m_body;
    // 响应原因
    std::string m_reason;
    // 响应头部MAP
    MapType m_headers;
    // cookies
    std::vector<std::string> m_cookies;
};

}
}

#endif
