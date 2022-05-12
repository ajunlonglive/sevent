#ifndef SEVENT_NET_SOCKETSOPSWIN_H
#define SEVENT_NET_SOCKETSOPSWIN_H
#ifdef _WIN32
#include <stdint.h>

namespace sevent {
namespace net {
namespace sockets {
int evutil_inet_pton(int af, const char *src, void *dst);
const char *evutil_inet_ntop(int af, const void *src, char *dst, size_t len);
} // namespace sockets
} // namespace net
} // namespace sevent

#endif
#endif