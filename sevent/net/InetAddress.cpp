#include "InetAddress.h"

#include "EndianOps.h"
#include "SocketsOps.h"
#include <string.h>
using namespace std;
using namespace sevent;
using namespace sevent::net;

static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");
InetAddress::InetAddress() {
    memset(&addr6, 0, sizeof(addr6));
}
// INADDR_ANY;  0.0.0.0
// INADDR_LOOPBACK; 127.0.0.1
InetAddress::InetAddress(uint16_t port , bool ipv6, bool loopback) {
    if (!ipv6) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        in_addr_t ip = loopback ? INADDR_LOOPBACK : INADDR_ANY;
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

InetAddress::InetAddress(const std::string& ip, uint16_t port, bool ipv6) {
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

string InetAddress::toStringIp() const {
    char buf[64]{0};
    if (addr.sin_family == AF_INET)
        sockets::inet_ntop(AF_INET, &(addr.sin_addr), buf, static_cast<socklen_t>(sizeof(buf)));
    else if (addr.sin_family == AF_INET6)
        sockets::inet_ntop(AF_INET6, &(addr6.sin6_addr), buf, static_cast<socklen_t>(sizeof(buf)));
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