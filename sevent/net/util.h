#ifndef SEVENT_NET_UTIL_H
#define SEVENT_NET_UTIL_H

#ifdef _WIN32
#include <stdint.h>
#include <basetsd.h>
#define ssize_t SSIZE_T
#endif

namespace sevent {
namespace net {
#ifndef _WIN32
    using socket_t = int;
#else
    using socket_t = intptr_t;
#endif    

} // namespace net
} // namespace sevent

#endif