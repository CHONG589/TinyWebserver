/**
 * @file address.h
 * @brief 网络地址的封装(IPv4, IPv6, Unix)
 * @author zch
 * @date 2025-03-29
 */

#ifndef ADDRESS_H__
#define ADDRESS_H__

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

/**
 * @brief 网络地址的基类,抽象类
 */
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    /**
     * @brief 通过sockaddr指针创建Address
     * @param[in] addr sockaddr指针
     * @param[in] addrlen sockaddr的长度
     * @return 返回和sockaddr相匹配的Address,失败返回nullptr
     */
    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

    /**
     * @brief 虚析构函数
     */
    virtual ~Address() { }

    /**
     * @brief 返回协议簇
     */
    int getFamily() const;

    /**
     * @brief 返回sockaddr指针,只读
     */
    virtual const sockaddr *getAddr() const = 0;

    /**
     * @brief 返回sockaddr指针,读写
     */
    virtual sockaddr *getAddr() = 0;

    /**
     * @brief 返回sockaddr的长度
     */
    virtual socklen_t getAddrLen() const = 0;

    /**
     * @brief 可读性输出地址
     */
    virtual std::ostream& insert(std::ostream& os) const = 0;

    /**
     * @brief 返回可读性字符串
     */
    std::string toString() const;

    /**
     * @brief 比较操作符
     */
    bool operator<(const Address& rhs) const;

    /**
     * @brief 比较操作符
     */
    bool operator==(const Address& rhs) const;

    /**
     * @brief 比较操作符
     */
    bool operator!=(const Address& rhs) const;
};

/**
 * @brief IP地址的基类
 */
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    /**
     * @brief 通过域名,IP,端口号创建IPAddress
     * @param[in] address 域名,IP
     * @param[in] port 端口号
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    static IPAddress::ptr Create(const char *address, uint16_t port = 0);

    /**
     * @brief 返回端口号
     */
    virtual uint32_t getPort() const = 0;

    /**
     * @brief 设置端口号
     */
    virtual void setPort(uint16_t v) = 0;
};

/**
 * @brief IPv4地址
 */
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    /**
     * @brief 使用点分十进制地址创建IPv4Address
     * @param[in] address 点分十进制地址,如:192.168.1.1
     * @param[in] port 端口号
     * @return 返回IPv4Address,失败返回nullptr
     */
    static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

    /**
     * @brief 通过sockaddr_in构造IPv4Address
     * @param[in] address sockaddr_in结构体
     */
    IPv4Address(const sockaddr_in &address);

    /**
     * @brief 通过二进制地址构造IPv4Address
     * @param[in] address 二进制地址address
     * @param[in] port 端口号
     */
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    // sockaddr_in结构体
    sockaddr_in m_addr;
};

/**
 * @brief IPv6地址
 */
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    /**
     * @brief 通过IPv6地址字符串构造IPv6Address
     * @param[in] address IPv6地址字符串
     * @param[in] port 端口号
     */
    static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

    /**
     * @brief 无参构造函数
     */
    IPv6Address();

    /**
     * @brief 通过sockaddr_in6构造IPv6Address
     * @param[in] address sockaddr_in6结构体
     */
    IPv6Address(const sockaddr_in6 &address);

    /**
     * @brief 通过IPv6二进制地址构造IPv6Address
     * @param[in] address IPv6二进制地址
     * @param[in] port 端口号
     */
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    // sockaddr_in6结构体
    sockaddr_in6 m_addr;
};

/**
 * @brief UnixSocket地址
 */
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    /**
     * @brief 无参构造函数
     */
    UnixAddress();

    /**
     * @brief 通过路径构造UnixAddress
     * @param[in] path UnixSocket路径(长度小于UNIX_PATH_MAX)
     */
    UnixAddress(const std::string &path);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::string getPath() const;
    std::ostream& insert(std::ostream& os) const override;

private:
    // sockaddr_un结构体
    sockaddr_un m_addr;
    // 地址长度
    socklen_t m_length;
};

/**
 * @brief 未知地址
 */
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;

    /**
     * @brief 通过协议簇构造UnknownAddress
     * @param[in] family 协议簇
     */
    UnknownAddress(int family);

    /**
     * @brief 通过sockaddr构造UnknownAddress
     * @param[in] addr sockaddr结构体
     */
    UnknownAddress(const sockaddr &addr);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    // sockaddr结构体
    sockaddr m_addr;
};

/**
 * @brief 流式输出Address
 */
std::ostream& operator<<(std::ostream& os, const Address& addr);

#endif
