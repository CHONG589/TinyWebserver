/**
 * @file http.cpp
 * @brief HTTP相关方法实现
 * @date 2026-03-29
 */

#include "http/transitionFiles/http.h"
#include "base/util.h"

namespace zch {
namespace http {

HttpMethod StringToHttpMethod(const std::string &m) {
#define XX(num, name, string)              \
    if (strcmp(#string, m.c_str()) == 0) { \
        return HttpMethod::name;           \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char *m) {
#define XX(num, name, string)                        \
    if (strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name;                     \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char *s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char *HttpMethodToString(const HttpMethod &m) {
    uint32_t idx = (uint32_t)m;
    if (idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

const char *HttpStatusToString(const HttpStatus &s) {
    switch (s) {
#define XX(code, name, msg) \
    case HttpStatus::name:  \
        return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
    default:
        return "<unknown>";
    }
}

bool CaseInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(HttpMethod::GET)
    , m_version(version)
    , m_close(close)
    , m_websocket(false)
    , m_parserParamFlag(0)
    , m_path("/") {
}

std::shared_ptr<HttpResponse> HttpRequest::CreateResponse() {
    //HttpResponse::ptr rsp(new HttpResponse(getVersion(), isClose()));
    //return rsp;
}

std::string HttpRequest::GetHeader(const std::string &key, const std::string &def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpRequest::InitQueryParam() {
    if (m_parserParamFlag & 0x1) {
        return;
    }

#define PARSE_PARAM(str, m, flag, trim)                                                                    \
    size_t pos = 0;                                                                                        \
    do {                                                                                                   \
        size_t last = pos;                                                                                 \
        pos         = str.find('=', pos);                                                                  \
        if (pos == std::string::npos) {                                                                    \
            break;                                                                                         \
        }                                                                                                  \
        size_t key = pos;                                                                                  \
        pos        = str.find(flag, pos);                                                                  \
                                                                                                           \
        if (0) {                                                                                           \
            std::cout << "<key>:" << str.substr(last, key - last)                                          \
                      << " <decoded>:" << sylar::StringUtil::UrlDecode(str.substr(last, key - last))       \
                      << " <value>:" << str.substr(key + 1, pos - key - 1)                                 \
                      << " <decoded>:" << sylar::StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1)) \
                      << std::endl;                                                                        \
        }                                                                                                  \
                                                                                                           \
        m.insert(std::make_pair(StringUtil::UrlDecode(trim(str.substr(last, key - last))),                 \
                                StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1))));               \
        if (pos == std::string::npos) {                                                                    \
            break;                                                                                         \
        }                                                                                                  \
        ++pos;                                                                                             \
    } while (true);

    PARSE_PARAM(m_query, m_params, '&', );
    m_parserParamFlag |= 0x1;
}

std::string HttpRequest::GetParam(const std::string &key, const std::string &def) {
    InitQueryParam();
    InitBodyParam();
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

void HttpRequest::InitBodyParam() {
    if (m_parserParamFlag & 0x2) {
        return;
    }
    std::string content_type = GetHeader("content-type");
    if (strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr) {
        m_parserParamFlag |= 0x2;
        return;
    }

    PARSE_PARAM(m_body, m_params, '&', );
    m_parserParamFlag |= 0x2;
}

std::string HttpRequest::GetCookie(const std::string &key, const std::string &def) {
    InitCookies();
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? def : it->second;
}

void HttpRequest::InitCookies() {
    if (m_parserParamFlag & 0x4) {
        return;
    }
    std::string cookie = GetHeader("cookie");
    if (cookie.empty()) {
        m_parserParamFlag |= 0x4;
        return;
    }
    PARSE_PARAM(cookie, m_cookies, ';', sylar::StringUtil::Trim);
    m_parserParamFlag |= 0x4;
}

void HttpRequest::SetHeader(const std::string &key, const std::string &val) {
    m_headers[key] = val;
}

void HttpRequest::SetParam(const std::string &key, const std::string &val) {
    m_params[key] = val;
}

void HttpRequest::SetCookie(const std::string &key, const std::string &val) {
    m_cookies[key] = val;
}

void HttpRequest::DelHeader(const std::string &key) {
    m_headers.erase(key);
}

void HttpRequest::DelParam(const std::string &key) {
    m_params.erase(key);
}

void HttpRequest::DelCookie(const std::string &key) {
    m_cookies.erase(key);
}

bool HttpRequest::HasHeader(const std::string &key, std::string *val) {
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        return false;
    }

    if (val) {
        *val = it->second;
    }

    return true;
}

bool HttpRequest::HasParam(const std::string &key, std::string *val) {
    InitQueryParam();
    InitBodyParam();
    auto it = m_params.find(key);
    if (it == m_params.end()) {
        return false;
    }

    if (val) {
        *val = it->second;
    }

    return true;
}

bool HttpRequest::HasCookie(const std::string &key, std::string *val) {
    InitCookies();
    auto it = m_cookies.find(key);
    if (it == m_cookies.end()) {
        return false;
    }

    if (val) {
        *val = it->second;
    }
    
    return true;
}

}
}
