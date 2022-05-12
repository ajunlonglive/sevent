#ifdef _WIN32
#include "sevent/net/SocketsOpsWin.h"
#include <stdio.h>
#include <string.h>
#include <ws2tcpip.h>


using namespace std;
using namespace sevent;
using namespace sevent::net;
namespace {
// from libevent
static const uint32_t EVUTIL_ISDIGIT_TABLE[8] = {0, 0x3ff0000, 0, 0, 0, 0, 0, 0};
static const uint32_t EVUTIL_ISXDIGIT_TABLE[8] = {0, 0x3ff0000, 0x7e, 0x7e, 0, 0, 0, 0};
int EVUTIL_ISDIGIT_(char c) {
    uint8_t u = c;
    return !!(EVUTIL_ISDIGIT_TABLE[(u >> 5) & 7] & (1U << (u & 31)));
}
int EVUTIL_ISXDIGIT_(char c) {
    uint8_t u = c;
    return !!(EVUTIL_ISXDIGIT_TABLE[(u >> 5) & 7] & (1U << (u & 31)));
}
}
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
int sockets::evutil_inet_pton(int af, const char *src, void *dst) {
    if (af == AF_INET) {
        unsigned a, b, c, d;
        char more;
        struct in_addr *addr = reinterpret_cast<struct in_addr *>(dst);
        if (sscanf(src, "%u.%u.%u.%u%c", &a, &b, &c, &d, &more) != 4)
            return 0;
        if (a > 255)
            return 0;
        if (b > 255)
            return 0;
        if (c > 255)
            return 0;
        if (d > 255)
            return 0;
        addr->s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
        return 1;
    } else if (af == AF_INET6) {
        struct in6_addr *out = reinterpret_cast<struct in6_addr *>(dst);
        uint16_t words[8];
        int gapPos = -1, i, setWords = 0;
        const char *dot = strchr(src, '.');
        const char *eow; /* end of words. */
        if (dot == src)
            return 0;
        else if (!dot)
            eow = src + strlen(src);
        else {
            unsigned byte1, byte2, byte3, byte4;
            char more;
            for (eow = dot - 1; eow >= src && EVUTIL_ISDIGIT_(*eow); --eow)
                ;
            ++eow;

            /* We use "scanf" because some platform inet_aton()s are too lax
             * about IPv4 addresses of the form "1.2.3" */
            if (sscanf(eow, "%u.%u.%u.%u%c", &byte1, &byte2, &byte3, &byte4,
                       &more) != 4)
                return 0;

            if (byte1 > 255 || byte2 > 255 || byte3 > 255 || byte4 > 255)
                return 0;

            words[6] = (byte1 << 8) | byte2;
            words[7] = (byte3 << 8) | byte4;
            setWords += 2;
        }

        i = 0;
        while (src < eow) {
            if (i > 7)
                return 0;
            if (EVUTIL_ISXDIGIT_(*src)) {
                char *next;
                long r = strtol(src, &next, 16);
                if (next > 4 + src)
                    return 0;
                if (next == src)
                    return 0;
                if (r < 0 || r > 65536)
                    return 0;

                words[i++] = (uint16_t)r;
                setWords++;
                src = next;
                if (*src != ':' && src != eow)
                    return 0;
                ++src;
            } else if (*src == ':' && i > 0 && gapPos == -1) {
                gapPos = i;
                ++src;
            } else if (*src == ':' && i == 0 && src[1] == ':' && gapPos == -1) {
                gapPos = i;
                src += 2;
            } else {
                return 0;
            }
        }

        if (setWords > 8 || (setWords == 8 && gapPos != -1) ||
            (setWords < 8 && gapPos == -1))
            return 0;

        if (gapPos >= 0) {
            int nToMove = setWords - (dot ? 2 : 0) - gapPos;
            int gapLen = 8 - setWords;
            /* assert(nToMove >= 0); */
            if (nToMove < 0)
                return -1; /* should be impossible */
            memmove(&words[gapPos + gapLen], &words[gapPos],
                    sizeof(uint16_t) * nToMove);
            memset(&words[gapPos], 0, sizeof(uint16_t) * gapLen);
        }
        for (i = 0; i < 8; ++i) {
            out->s6_addr[2 * i] = words[i] >> 8;
            out->s6_addr[2 * i + 1] = words[i] & 0xff;
        }
        return 1;
    } else {
        return -1;
    }
}
const char *sockets::evutil_inet_ntop(int af, const void *src, char *dst, size_t len) {
    if (af == AF_INET) {
        const struct in_addr *in =
            reinterpret_cast<const struct in_addr *>(src);
        const uint32_t a = ntohl(in->s_addr);
        int r;
        r = snprintf(dst, len, "%d.%d.%d.%d", (int)(uint8_t)((a >> 24) & 0xff),
                     (int)(uint8_t)((a >> 16) & 0xff),
                     (int)(uint8_t)((a >> 8) & 0xff), (int)(uint8_t)((a)&0xff));
        if (r < 0 || (size_t)r >= len)
            return NULL;
        else
            return dst;
    } else if (af == AF_INET6) {
        const struct in6_addr *addr =
            reinterpret_cast<const struct in6_addr *>(src);
        char buf[64], *cp;
        int longestGapLen = 0, longestGapPos = -1, i, curGapPos = -1,
            curGapLen = 0;
        uint16_t words[8];
        for (i = 0; i < 8; ++i) {
            words[i] = (((uint16_t)addr->s6_addr[2 * i]) << 8) +
                       addr->s6_addr[2 * i + 1];
        }
        if (words[0] == 0 && words[1] == 0 && words[2] == 0 && words[3] == 0 &&
            words[4] == 0 &&
            ((words[5] == 0 && words[6] && words[7]) || (words[5] == 0xffff))) {
            /* This is an IPv4 address. */
            if (words[5] == 0) {
                snprintf(buf, sizeof(buf), "::%d.%d.%d.%d", addr->s6_addr[12],
                         addr->s6_addr[13], addr->s6_addr[14],
                         addr->s6_addr[15]);
            } else {
                snprintf(buf, sizeof(buf), "::%x:%d.%d.%d.%d", words[5],
                         addr->s6_addr[12], addr->s6_addr[13],
                         addr->s6_addr[14], addr->s6_addr[15]);
            }
            if (strlen(buf) > len)
                return NULL;
            strncpy(dst, buf, len);
            return dst;
        }
        i = 0;
        while (i < 8) {
            if (words[i] == 0) {
                curGapPos = i++;
                curGapLen = 1;
                while (i < 8 && words[i] == 0) {
                    ++i;
                    ++curGapLen;
                }
                if (curGapLen > longestGapLen) {
                    longestGapPos = curGapPos;
                    longestGapLen = curGapLen;
                }
            } else {
                ++i;
            }
        }
        if (longestGapLen <= 1)
            longestGapPos = -1;

        cp = buf;
        for (i = 0; i < 8; ++i) {
            if (words[i] == 0 && longestGapPos == i) {
                if (i == 0)
                    *cp++ = ':';
                *cp++ = ':';
                while (i < 8 && words[i] == 0)
                    ++i;
                --i; /* to compensate for loop increment. */
            } else {
                snprintf(cp, sizeof(buf) - (cp - buf), "%x",
                         (unsigned)words[i]);
                cp += strlen(cp);
                if (i != 7)
                    *cp++ = ':';
            }
        }
        *cp = '\0';
        if (strlen(buf) > len)
            return NULL;
        strncpy(dst, buf, len);
        return dst;
    } else {
        return NULL;
    }
}
#pragma GCC diagnostic warning "-Wold-style-cast"
#pragma GCC diagnostic warning "-Wconversion"
#endif