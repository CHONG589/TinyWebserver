#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "address.h"
#include "endian.h"
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
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            //LOG_ERROR("unknown address family: %d", addr->sa_family);
            break;
    }
    return result;
}

bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                    int family, int type, int protocol) {

    addrinfo hints, *results, *next;
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
        //LOG_ERROR("Address::Lookup getaddress() error!");
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
    if (Lookup(result, host, family, type, protocol)) {
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
    else if (getAddrLen() < rhs.getAddrLen()) {
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

IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, NULL, &hints, &results);
    if (error) {
        //LOG_ERROR("IPAddress::Create() error!");
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

IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    ////将点分十进制的ip地址转化为用于网络传输的数值格式,存储在rt->m_addr.sin_addr中
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0) {
        //LOG_ERROR("IPv4Address::Crete() error!");
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in &address) {
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    //这里因为传入的参数地址本来就是用二进制的形式传入的，不像上面Create中的是
    //const char *型地址，要进行转换为网络序，这里只需转换为小端就行。
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

sockaddr *IPv4Address::getAddr(){
    return (sockaddr *)&m_addr;
}

const sockaddr *IPv4Address::getAddr() const {
    return (sockaddr *)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

//将地址转换成可读的形式并输出地址，即转换成点分十进制的形式
std::ostream &IPv4Address::insert(std::ostream &os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    //& 0xff表示只保留低位的8位
    os << ((addr >> 24) & 0xff) << '.'
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff)  << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if (result <= 0) {
        //LOG_ERROR("IPv6Address::Create() error!");
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address) {
    m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

sockaddr *IPv6Address::getAddr() {
    return (sockaddr *)&m_addr;
}

const sockaddr *IPv6Address::getAddr() const {
    return (sockaddr *)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream &IPv6Address::insert(std::ostream &os) const {
    os << "[";
    //IPv6的地址形式为每16位一个 : 
    uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
    //用于控制只能使用一次::
    bool used_zeros = false;
    //IPv6共128位，分为8组，每组16位
    //1. 为了书写方便，每组中的前导“0”都可以省略
    //例：2001:db8:130F:0000:0000:09C0:876A:130B
    //可写成：2001:db8:130F:0:0:9C0:876A:130B
    //2. 地址中包含的连续两个或多个均为0的组，可以用双冒号“::”来代替
    //所以上面的地址可以进一步写成：
    //2001:db8:130F::9C0:876A:130B
    //在一个IPv6地址中只能使用一次双冒号“::”，否则当计算机将压缩后的
    //地址恢复成128位时，无法确定每段中0的个数
    for (size_t i = 0; i < 8; ++i) {
        if (addr[i] == 0 && !used_zeros) {
            continue;
        }
        if (i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if (i) {
            os << ":";
        }
        //std::hex表示后面一个参数用十六进制的方式输出
        //std::dec是用十进制的方式输出 
        os << std::hex <<(int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if (!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

std::ostream &operator<<(std::ostream &os, const Address &addr) {
    return addr.insert(os);
}

}//namespace zch
