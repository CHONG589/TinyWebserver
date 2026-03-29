/**
 * @file uri.h
 * @brief url 相关方法的封装, 基于 nodejs/http-parser
 * @author zch
 * @date 2026-02-27
 */

#include "base/uri.h"

#include <sstream>

static zch::Logger::ptr g_logger = LOG_NAME("system");

/**
 * @brief 创建 Uri 对象
 * @param[in] uri uri 字符串
 * @return 解析成功返回 Uri 对象否则返回 nullptr
 */
Uri::ptr Uri::Create(const std::string &urlStr) {

    Uri::ptr uri(new Uri);
    struct http_parser_url parser;

    // 解析 url 字符串
    if (http_parser_parse_url(urlStr.c_str(), urlStr.length(), 0, &parser) != 0) {
        LOG_WARN(g_logger) << "http_parser_parse_url failed, urlStr: " << urlStr;
        return nullptr;
    }

    // 检查 scheme 是否存在，和提取内容
    // (1 << UF_SCHEMA) ：通过位移操作生成一个只在 UF_SCHEMA 位为 1 的掩码
    // & 运算 ：如果 field_set 的对应位也是 1，说明解析出的 URL 中包含了协议部分。

    // http_parser 库为了高效（零拷贝），解析时不会生成新的字符串，而是记录各个部分在原始字符串中的位置和长度。
    // parser.field_data[UF_SCHEMA] ：这是一个结构体，存储了协议部分的元数据。
    // .off (Offset) ：协议字符串在原始 urlStr 中的起始偏移量。
    // .len (Length) ：协议字符串的长度。
    if (parser.field_set & (1 << UF_SCHEMA)) {
        uri->SetScheme(std::string(urlStr.c_str() + parser.field_data[UF_SCHEMA].off,
                        parser.field_data[UF_SCHEMA].len));
    }

    // 检查 userInfo 是否存在，和提取内容
    if (parser.field_set & (1 << UF_USERINFO)) {
        uri->SetUserInfo(std::string(urlStr.c_str() + parser.field_data[UF_USERINFO].off,
                        parser.field_data[UF_USERINFO].len));
    }

    // 检查 host 是否存在，和提取内容
    if (parser.field_set & (1 << UF_HOST)) {
        uri->SetHost(std::string(urlStr.c_str() + parser.field_data[UF_HOST].off,
                        parser.field_data[UF_HOST].len));
    }

    // 检查 port 是否存在，和提取内容
    if (parser.field_set & (1 << UF_PORT)) {
        uri->SetPort(std::stoi(std::string(urlStr.c_str() + parser.field_data[UF_PORT].off,
                        parser.field_data[UF_PORT].len)));
    } else {
        // 默认端口号解析只支持 http/ws/https
        if (uri->GetScheme() == "http" || uri->GetScheme() == "ws") {
            uri->SetPort(80);
        } else if (uri->GetScheme() == "https") {
            uri->SetPort(443);
        }
    }

    // 检查 path 是否存在，和提取内容
    if (parser.field_set & (1 << UF_PATH)) {
        uri->SetPath(std::string(urlStr.c_str() + parser.field_data[UF_PATH].off,
                        parser.field_data[UF_PATH].len));
    }

    // 检查 query 是否存在，和提取内容
    if (parser.field_set & (1 << UF_QUERY)) {
        uri->SetQuery(std::string(urlStr.c_str() + parser.field_data[UF_QUERY].off,
                        parser.field_data[UF_QUERY].len));
    }

    // 检查 fragment 是否存在，和提取内容
    if (parser.field_set & (1 << UF_FRAGMENT)) {
        uri->SetFragment(std::string(urlStr.c_str() + parser.field_data[UF_FRAGMENT].off,
                        parser.field_data[UF_FRAGMENT].len));
    }

    return uri;
}

/**
* @brief 返回端口
*/
int32_t Uri::GetPort() const {
    if (m_port) {
        return m_port;
    }

    if (m_scheme == "http" || m_scheme == "ws") {
        return 80;
    } else if (m_scheme == "https" || m_scheme == "wss") {
        return 443;
    }

    return m_port;
}

/**
* @brief 返回路径
*/
const std::string &Uri::GetPath() const {
    static std::string s_default_path = "/";
    return m_path.empty() ? s_default_path : m_path;
}

/**
* @brief 是否默认端口
*/
bool Uri::IsDefaultPort() const {
    if (m_scheme == "http" || m_scheme == "ws") {
        return m_port == 80;
    } else if (m_scheme == "https") {
        return m_port == 443;
    } else {
        return false;
    }
}

/**
* @brief 转成字符串
*/
std::string Uri::ToString() const {
    std::stringstream ss;

    ss << m_scheme
       << "://"
       << m_userInfo
       << (m_userInfo.empty() ? "" : "@")
       << m_host << (IsDefaultPort() ? "" : ":" + std::to_string(m_port))
       << GetPath()
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << m_fragment;

    return ss.str();
}

/**
* @brief 获取 Address
*/
Address::ptr Uri::CreateAddress() const {
    auto addr = Address::LookupAnyIPAddress(m_host);
    if(addr) {
        addr->setPort(GetPort());
    }
    return addr;
}
