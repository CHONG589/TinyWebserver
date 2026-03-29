/**
 * @file uri.h
 * @brief url 相关方法的封装
 * @author zch
 * @date 2026-02-27
 */

#ifndef URI_H__
#define URI_H__

#include <memory>
#include <string>
#include <stdint.h>

#include "address.h"
#include "http/http-parser/http_parser.h"

/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/

// 关于 url 组成的讲解 https://developer.mozilla.org/zh-CN/docs/Learn_web_development/Howto/Web_mechanics/What_is_a_URL

class Uri {
public:
    // 智能指针类型定义
    typedef std::shared_ptr<Uri> ptr;

    /**
     * @brief 创建 Uri 对象
     * @param[in] urlStr uri 字符串
     * @return 解析成功返回 Uri 对象否则返回 nullptr
     */
    static Uri::ptr Create(const std::string& urlStr);

    /**
     * @brief 构造函数
     */
    Uri() : m_port(0) {}

    /**
     * @brief 返回 scheme(方案)
     */
    const std::string& GetScheme() const { return m_scheme;}

    /**
     * @brief 返回用户信息
     */
    const std::string& GetUserInfo() const { return m_userInfo;}

    /**
     * @brief 返回 host
     */
    const std::string& GetHost() const { return m_host;}

    /**
     * @brief 返回路径
     */
    const std::string& GetPath() const;

    /**
     * @brief 返回查询条件
     */
    const std::string& GetQuery() const { return m_query;}

    /**
     * @brief 返回 fragment(片段标识符)
     */
    const std::string& GetFragment() const { return m_fragment;}

    /**
     * @brief 返回端口
     */
    int32_t GetPort() const;

    /**
     * @brief 设置用户信息
     * @param[in] v 用户信息
     */
    void SetUserinfo(const std::string& v) { m_userInfo = v;}

    /**
     * @brief 设置host信息
     * @param[in] v host
     */
    void SetHost(const std::string& v) { m_host = v;}

    /**
     * @brief 设置路径
     * @param[in] v 路径
     */
    void SetPath(const std::string& v) { m_path = v;}

    /**
     * @brief 设置查询条件
     * @param[in] v
     */
    void SetQuery(const std::string& v) { m_query = v;}

    /**
     * @brief 设置fragment
     * @param[in] v fragment
     */
    void SetFragment(const std::string& v) { m_fragment = v;}

    /**
     * @brief 设置端口号
     * @param[in] v 端口
     */
    void SetPort(int32_t v) { m_port = v;}

    /**
     * @brief 序列化到输出流
     * @param[in out] os 输出流
     * @return 输出流
     */
    std::ostream& Dump(std::ostream& os) const;

    /**
     * @brief 转成字符串
     */
    std::string ToString() const;

    /**
     * @brief 获取 Address
     */
    Address::ptr CreateAddress() const;

private:
    /**
     * @brief 是否默认端口
     */
    bool IsDefaultPort() const;

private:
    // schema(方案)
    std::string m_scheme;
    // 用户信息
    std::string m_userInfo;
    // host
    std::string m_host;
    // 路径
    std::string m_path;
    // 查询参数
    std::string m_query;
    // fragment(片段标识符)
    std::string m_fragment;
    // 端口
    int32_t m_port;
};

#endif
