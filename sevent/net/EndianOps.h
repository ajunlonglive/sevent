#ifndef SEVENT_NET_ENDIAN_H
#define SEVENT_NET_ENDIAN_H

#include <endian.h>
#include <stdint.h>

namespace sevent {
namespace net {
namespace sockets {
inline uint64_t hostToNet64(uint64_t host64) { return htobe64(host64); }

inline uint32_t hostToNet32(uint32_t host32) { return htobe32(host32); }

inline uint16_t hostToNet16(uint16_t host16) { return htobe16(host16); }

inline uint64_t netToHost64(uint64_t net64) { return be64toh(net64); }

inline uint32_t netToHost32(uint32_t net32) { return be32toh(net32); }

inline uint16_t netToHost16(uint16_t net16) { return be16toh(net16); }

} // namespace sockets
} // namespace net
} // namespace sevent

#endif