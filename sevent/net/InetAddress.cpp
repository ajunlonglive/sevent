#include "sevent/net/InetAddress.h"

#include "sevent/base/Logger.h"
#include "sevent/net/EndianOps.h"
#include "sevent/net/SocketsOps.h"
#include <assert.h>
#ifndef _WIN32
#include <netdb.h>
#else
#endif
#include <string.h>
#include <thread>
using namespace std;
using namespace sevent;
using namespace sevent::net;

static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");
InetAddress::InetAddress() { memset(&addr6, 0, sizeof(addr6)); }
// INADDR_ANY;  0.0.0.0
// INADDR_LOOPBACK; 127.0.0.1
InetAddress::InetAddress(uint16_t port, bool ipv6, bool loopback) {
    if (!ipv6) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        uint32_t ip = loopback ? INADDR_LOOPBACK : INADDR_ANY;
        addr.sin_addr.s_addr = sockets::hostToNet32(ip);
        addr.sin_port = sockets::hostToNet16(port);
    } else {
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        in6_addr ip = loopback ? in6addr_loopback : in6addr_any;
        addr6.sin6_addr = ip;
        addr6.sin6_port = sockets::hostToNet16(port);
    }
}

InetAddress::InetAddress(uint16_t port, const std::string &ip, bool ipv6) {
    if (!ipv6) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = sockets::hostToNet16(port);
        sockets::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    } else {
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = sockets::hostToNet16(port);
        sockets::inet_pton(AF_INET6, ip.c_str(), &addr6.sin6_addr);
    }
}
InetAddress::InetAddress(const std::string& ip, uint16_t port, bool ipv6)
    : InetAddress(port, ip, ipv6) {}
InetAddress::InetAddress(uint16_t port, const char *ip, bool ipv6)
    : InetAddress(port, string(ip), ipv6) {}
InetAddress::InetAddress(const char* ip, uint16_t port, bool ipv6)
    : InetAddress(port, string(ip), ipv6) {}

string InetAddress::toStringIp() const {
    char buf[64]{0};
    if (addr.sin_family == AF_INET)
        sockets::inet_ntop(AF_INET, &(addr.sin_addr), buf,
                           static_cast<socklen_t>(sizeof(buf)));
    else if (addr.sin_family == AF_INET6)
        sockets::inet_ntop(AF_INET6, &(addr6.sin6_addr), buf,
                           static_cast<socklen_t>(sizeof(buf)));
    return buf;
}
string InetAddress::toStringIpPort() const {
    string ipstr;
    ipstr.reserve(64);
    ipstr = toStringIp();
    ipstr.append(":");
    ipstr.append(to_string(getPortHost()));
    return ipstr;
}
uint16_t InetAddress::getPortHost() const {
    return sockets::netToHost16(addr.sin_port);
}

bool InetAddress::resolve(const std::string& hostname, InetAddress *iaddr) {
    bool success = false;
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    int ret = ::getaddrinfo(hostname.c_str(), NULL, &hints, &result);
    if (ret == 0 && result != NULL) {
        iaddr->addr.sin_addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr;
        success = true;
    } else {
        if (ret)
            LOG_SYSERR << "InetAddress::resolve() - "<< gai_strerror(ret);
        success = false;
    }
    ::freeaddrinfo(result); /* No longer needed */
    return success;
}