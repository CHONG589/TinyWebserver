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

class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    //通过传的参数 addr 创建 Address，返回和 sockaddr 相匹配的 Address 指针。
    //通过 addr 中的 family 确定具体的地址（如 IPv4），然后用具体的地址类去
    //创建实例。
    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);
    virtual ~Address() { }
    int getFamily() const;
    //子类必须实现，纯虚函数
    virtual const sockaddr *getAddr() const = 0;
    virtual sockaddr *getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;
    
    // 添加输出流重载
    friend std::ostream& operator<<(std::ostream& os, const Address& addr);
    virtual std::ostream& insert(std::ostream& os) const = 0;
};

/**
 * @brief IP地址的基类
 */
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;
    //通过域名，IP创建 IPAddress
    static IPAddress::ptr Create(const char *host, uint16_t port = 0);
    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};

/**
 * @brief IPv4 地址
 */
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;
    //使用点分十进制地址创建 IPv4Address
    static IPv4Address::ptr Create(const char *address, uint16_t port = 0);
    //通过 sockaddr_in 构造 IPv4Address
    IPv4Address(const sockaddr_in &address);
    //通过二进制地址构造 IPv4Address
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_in m_addr;
};

/**
 * @brief IPv6 地址
 */
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char *address, uint16_t port = 0);
    IPv6Address();
    IPv6Address(const sockaddr_in6 &address);
    //通过 IPv6 二进制地址构造 IPv6Address
    IPv6Address(const uint8_t address[16], uint16_t port = 0);
    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_in6 m_addr;
};

/**
 * @brief UnixSocket地址
 */
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string &path);
    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::string getPath() const;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

/**
 * @brief 未知地址
 */
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr &addr);
    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr m_addr;
};

#endif
