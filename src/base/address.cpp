#include <netdb.h>
#include <errno.h>
#include <sstream>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "base/address.h"
#include "base/endian.h"
#include "zchlog.h"

/**
 * @brief 通过sockaddr指针创建Address
 * @param[in] addr sockaddr指针
 * @param[in] addrlen sockaddr的长度
 * @return 返回和sockaddr相匹配的Address,失败返回nullptr
 */
Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

/**
 * @brief 返回协议簇
 */
int Address::getFamily() const {
    return getAddr()->sa_family;
}

/**
 * @brief 返回可读性字符串
 */
std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

/**
 * @brief 比较操作符
 */

// IPAddress::ptr IPAddress::Create(const char *host, uint16_t port) {
//
// }

/**
 * @brief 使用点分十进制地址创建IPv4Address
 * @param[in] address 点分十进制地址,如:192.168.1.1
 * @param[in] port 端口号
 * @return 返回IPv4Address,失败返回nullptr
 */
IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    //网络字节序是大端序
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0) {
        LOG_WARN() << "IPv4Address::Create() error: " << strerror(errno);
        return nullptr;
    }
    return rt;
}

/**
 * @brief 通过sockaddr_in构造IPv4Address
 * @param[in] address sockaddr_in结构体
 */
IPv4Address::IPv4Address(const sockaddr_in &address) {
    m_addr = address;
}

/**
 * @brief 通过二进制地址构造IPv4Address
 * @param[in] address 二进制地址address
 * @param[in] port 端口号
 */
IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

/**
 * @brief 返回sockaddr指针,读写
 */
sockaddr *IPv4Address::getAddr() {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr指针,只读
 */
const sockaddr *IPv4Address::getAddr() const {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr的长度
 */
socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

/**
 * @brief 返回端口号
 */
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

/**
 * @brief 设置端口号
 */
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

/**
 * @brief 通过IPv6地址字符串构造IPv6Address
 * @param[in] address IPv6地址字符串
 * @param[in] port 端口号
 */
IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        LOG_WARN() << "IPv6Address::Create() error: " << strerror(errno);
        return nullptr;
    }
    return rt;
}

/**
 * @brief 无参构造函数
 */
IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

/**
 * @brief 通过sockaddr_in6构造IPv6Address
 * @param[in] address sockaddr_in6结构体
 */
IPv6Address::IPv6Address(const sockaddr_in6 &address) {
    m_addr = address;
}

/**
 * @brief 通过IPv6二进制地址构造IPv6Address
 * @param[in] address IPv6二进制地址
 * @param[in] port 端口号
 */
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    //由于数组中每个元素都时一字节的，所以不用转换字节序。
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

/**
 * @brief 返回sockaddr指针,读写
 */
sockaddr *IPv6Address::getAddr() {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr指针,只读
 */
const sockaddr *IPv6Address::getAddr() const {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr的长度
 */
socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

/**
 * @brief 返回端口号
 */
uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

/**
 * @brief 设置端口号
 */
void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

/**
 * @brief 无参构造函数
 */
UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

/**
 * @brief 通过路径构造UnixAddress
 * @param[in] path UnixSocket路径(长度小于UNIX_PATH_MAX)
 */
UnixAddress::UnixAddress(const std::string &path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

/**
 * @brief 设置地址长度
 */
void UnixAddress::setAddrLen(uint32_t v) {
    m_length = v;
}

/**
 * @brief 返回sockaddr指针,读写
 */
sockaddr *UnixAddress::getAddr() {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr指针,只读
 */
const sockaddr *UnixAddress::getAddr() const {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr的长度
 */
socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

/**
 * @brief 返回路径
 */
std::string UnixAddress::getPath() const {
    std::stringstream ss;
    if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
    } 
    else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

/**
 * @brief 通过协议簇构造UnknownAddress
 * @param[in] family 协议簇
 */
UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

/**
 * @brief 通过sockaddr构造UnknownAddress
 * @param[in] addr sockaddr结构体
 */
UnknownAddress::UnknownAddress(const sockaddr &addr) {
    m_addr = addr;
}

/**
 * @brief 返回sockaddr指针,读写
 */
sockaddr *UnknownAddress::getAddr() {
    return (sockaddr *)&m_addr;
}

/**
 * @brief 返回sockaddr指针,只读
 */
const sockaddr *UnknownAddress::getAddr() const {
    return &m_addr;
}

/**
 * @brief 返回sockaddr的长度
 */
socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

/**
 * @brief 可读性输出地址
 */
std::ostream& Address::insert(std::ostream& os) const {
    return os;
}

/**
 * @brief 流式输出Address
 */
std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

/**
 * @brief 可读性输出IPv4Address
 */
std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

/**
 * @brief 可读性输出IPv6Address
 */
std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i-1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

/**
 * @brief 可读性输出UnixAddress
 */
std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

/**
 * @brief 可读性输出UnknownAddress
 */
std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}
