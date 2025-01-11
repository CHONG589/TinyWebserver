#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "address.h"
#include "./log/log.h"

namespace zch {

/**
 * @brief 计算字节数
 */
template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for (; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if (addr == nullptr)
        return nullptr;
    
    Address::ptr result;
    switch (addr->sa_family) {
        case AF_INET:
            result = reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result = reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            LOG_ERROR("unknown address family: %d", addr->sa_family);
            break;
    }
    return result;
}

bool Address::Lookup(std::vector<Address:ptr> &result, const std::std::string &host,
                    int family, int type, int protocol) {

    addrinfo hints, &results, &next;
    hints.ai_flags     = 0;
    hints.ai_family    = family;
    hints.ai_socktype  = type;
    hints.ai_protocol  = protocol;
    hints.ai_addrlen   = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;

    std::string node;
    const char *service = NULL;
    //在参数 str 所指向的字符串的前 n 个字节中搜索第一次出现字符 c（一个无符号字符）的位置。
    //该函数返回一个指向匹配字节的指针，如果在给定的内存区域未出现字符，则返回 NULL。

    //检查是否为IPv6地址
    /**
     * 为了在一个URL中使用一文本IPv6地址，文本地址应该用符号“[”和“]”来封闭。
     * 例如：IPv6地址为：FEDC:BA98:7654:3210:FEDC:BA98:7654:3210
     * 用URL表示时是这样的：
     * http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html
     */
    if (!host.empty() && host[0] == '[') {
        //在第一个参数所指向的字符串的前host.size() - 1 个字节中搜索第一次出现字符 ']'的位置。
        //该函数返回一个指向匹配字节的指针，如果在给定的内存区域未出现字符，则返回 NULL。
        const char *endipv6 = (const char *)memchr(host.c_str(), ']', host.size() - 1);
        if (endipv6) {
            if (*(endipv6 + 1) == ':') {
                //去IPv6的端口
                service = endipv6 + 2;
            }
            //主要功能是复制子字符串，要求从指定位置开始，并具有指定的长度。
            //第一个参数为起始位置，第二个参数是复制的字符数目
            //取IPv6地址，即[]内的内容
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //如果IPv6地址为空则为IPv4服务
    //http://10.43.159.11:8080
    if (node.empty()) {
        service = (const char *)memchr(host.c_str(), ':', host.size());
        if (service) {
            if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                //这里表示没有第二个 ':'
                //取IPv4地址
                node = host.substr(0, service - host.c_str());
                //取IPv4端口
                ++service;
            }
        }
    }

    if (node.empty()) {
        node = host;
    }
    //获取host的hints类型的套接字(hostname(IPv4 || IPv6) service(服务器名或端口号))
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        LOG_ERROR("Address::Lookup getaddress() error!");
        return false;
    }
    next = results;
    while (next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}

Address::ptr Address::LookupAny(const std::string &host, int family, 
                                int type, int protocol) {
    
    std::vector<Address::ptr> result;
    if (Loopup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                           int type, int protocol) {

    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        for (auto &i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }
    return nullptr;
}

int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address &rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0) {
        return true;
    }
    else if (result > 0) {
        return false;
    }
    else if (getAddrLen() < rhs.getAddrlen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address &rhs) const {
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const {
    return !(*this == rhs);
}

IPAddress::ptr IPAaddress::Create(const char *address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddinfo(address, NULL, &hints, &result);
    if (error) {
        LOG_ERROR("IPAddress::Create() error!");
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if (result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } 
    catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

}//namespace zch
