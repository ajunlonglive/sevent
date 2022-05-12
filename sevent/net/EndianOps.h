#ifndef SEVENT_NET_ENDIAN_H
#define SEVENT_NET_ENDIAN_H
#ifndef _WIN32
#include <endian.h>
#else
#include <winsock2.h>
#endif
#include <stdint.h>

namespace sevent {
namespace net {
namespace sockets {
#ifndef _WIN32
inline uint64_t hostToNet64(uint64_t host64) { return htobe64(host64); }
inline uint32_t hostToNet32(uint32_t host32) { return htobe32(host32); }
inline uint16_t hostToNet16(uint16_t host16) { return htobe16(host16); }
inline uint64_t netToHost64(uint64_t net64) { return be64toh(net64); }
inline uint32_t netToHost32(uint32_t net32) { return be32toh(net32); }
inline uint16_t netToHost16(uint16_t net16) { return be16toh(net16); }
#else
#pragma GCC diagnostic ignored "-Wconversion"
inline uint64_t htonll(uint64_t val) {
    if (1 == htonl(1))
        return val;
    return ((static_cast<uint64_t>(htonl(val))) << 32) + htonl(val >> 32);
}

inline uint64_t ntohll(uint64_t val) {
    if (1 == htonl(1))
        return val;
    return ((static_cast<uint64_t>(ntohl(val))) << 32) + ntohl(val >> 32);
}
#pragma GCC diagnostic warning "-Wconversion"
inline uint64_t hostToNet64(uint64_t host64) { return htonll(host64); }
inline uint32_t hostToNet32(uint32_t host32) { return htonl(host32); }
inline uint16_t hostToNet16(uint16_t host16) { return htons(host16); }
inline uint64_t netToHost64(uint64_t net64) { return ntohll(net64); }
inline uint32_t netToHost32(uint32_t net32) { return ntohl(net32); }
inline uint16_t netToHost16(uint16_t net16) { return ntohs(net16); }
#endif
} // namespace sockets
} // namespace net
} // namespace sevent

#endif