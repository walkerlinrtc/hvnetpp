#include "hvnetpp/InetAddress.h"
#include "hvnetpp/SocketsOps.h"
#include <netdb.h>
#include <strings.h>
#include <cstring>
#include <cstddef>
#include <cassert>
#include <arpa/inet.h>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

namespace hvnetpp {

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) {
    if (ipv6) {
        bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htons(port);
    } else {
        bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = htonl(ip);
        addr_.sin_port = htons(port);
    }
}

InetAddress::InetAddress(std::string ip, uint16_t port, bool ipv6) {
    if (ipv6) {
        bzero(&addr6_, sizeof addr6_);
        sockets::fromIpPort(ip.c_str(), port, &addr6_);
    } else {
        bzero(&addr_, sizeof addr_);
        sockets::fromIpPort(ip.c_str(), port, &addr_);
    }
}

std::string InetAddress::toIpPort() const {
    char buf[64] = "";
    sockets::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}

std::string InetAddress::toIp() const {
    char buf[64] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

uint32_t InetAddress::ipNetEndian() const {
    assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const {
    return ntohs(portNetEndian());
}

bool InetAddress::resolve(std::string hostname, InetAddress* out) {
    assert(out != NULL);
    struct hostent* hent = gethostbyname(hostname.c_str());
    if (hent) {
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(hent->h_addr);
        return true;
    } else {
        // try getaddrinfo?
        return false;
    }
}

} // namespace hvnetpp
