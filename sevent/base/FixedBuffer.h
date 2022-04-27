#ifndef SEVENT_BASE_FIXEDBUFFER_H
#define SEVENT_BASE_FIXEDBUFFER_H
#include "noncopyable.h"
#include <string.h>

namespace sevent {

template <int SIZE> class FixedBuffer : noncopyable {
public:
    FixedBuffer() : cur(buffer){}
    void append(const char *buf, int len) {
        if (remain() > len) {
            memcpy(cur, buf, len);
            cur += len;
        }
    }
    bool empty() { return cur == buffer; }
    void add(size_t len) { cur += len; }
    void bzero() { memset(buffer, 0, sizeof(buffer)); }
    void reset() { cur = buffer; }
    int remain() { return static_cast<int>(end() - cur); }
    int size() { return static_cast<int>(cur - buffer); }
    int length() { return static_cast<int>(cur - buffer); }
    char *current() { return cur; }
    const char *begin() { return buffer; }
    const char *end() { return buffer + sizeof(buffer); }

private:
    char buffer[SIZE];
    char *cur;
};

} // namespace sevent

#endif