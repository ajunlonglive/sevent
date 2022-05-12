#ifndef SEVENT_BASE_FIXEDBUFFER_H
#define SEVENT_BASE_FIXEDBUFFER_H
#include "sevent/base/noncopyable.h"
#include <string.h>
#include <stdio.h>
#include <string>

namespace sevent {

template <int SIZE> class FixedBuffer : noncopyable {
public:
    FixedBuffer() : cur(buffer) {}
    void append(const char *buf, int len) {
        if (remain() > len) {
            memcpy(cur, buf, len);
            cur += len;
        }
    }
    bool empty() const { return cur == buffer; }
    void add(size_t len) { cur += len; }
    void bzero() { memset(buffer, 0, sizeof(buffer)); }
    void reset() { cur = buffer; }
    int remain() const { return static_cast<int>(end() - cur); }
    int size() const { return static_cast<int>(cur - buffer); }
    int length() const { return static_cast<int>(cur - buffer); }
    char *current() const { return cur; }
    const char *begin() const { return buffer; }
    const char *end() const { return buffer + sizeof(buffer); }

    std::string toString() const { return std::string(buffer, length()); }

private:
    char buffer[SIZE];
    char *cur;
};

} // namespace sevent

#endif