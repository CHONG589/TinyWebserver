#include "socket.h"
#include "./log/log.h"

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected(false) {
}

Socket::ptr Socket::CreateTCP(Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}
