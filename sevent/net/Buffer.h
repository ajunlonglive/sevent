#ifndef SEVENT_NET_BUFFER_H
#define SEVENT_NET_BUFFER_H

#include <stdint.h>
#include <string>
#include <vector>

namespace sevent {
namespace net {
class Buffer {
public:
    static const size_t prepends = 8;
    static const size_t initialSize = 1024;

    explicit Buffer(size_t initSize = initialSize)
        : buffer(prepends + initSize), readIndex(prepends),
          writeIndex(prepends) {}

    size_t readableBytes() const { return writeIndex - readIndex; }
    size_t writableBytes() const { return buffer.size() - writeIndex; }
    size_t prependableBytes() const { return readIndex; }

    const char *peek() const { return begin() + readIndex; }
    void append(const std::string &str);
    void append(const void *data, size_t len);
    void prepend(const void *data, size_t len);

    // 手动移动readIndex;
    void retrieve(size_t len);
    void retrieveAll();
    void swap(Buffer &buf);
    void shrinkToFit();

    size_t readBytes(char *buf, size_t len);
    std::string readAsString(size_t len);
    std::string readAllAsString();

    const char *findEOL() const;
    const char *findEOL(const char *start) const;
    const char *findCRLF() const;
    const char *findCRLF(const char *start) const;

    // TODO 大端
    int64_t peekInt64() const;
    int32_t peekInt32() const;
    int16_t peekInt16() const;
    int8_t  peekInt8()  const;
    int64_t readInt64();
    int32_t readInt32();
    int16_t readInt16();
    int8_t  readInt8();
    void appendInt64(int64_t n);
    void appendInt32(int32_t n);
    void appendInt16(int16_t n);
    void appendInt8(int8_t n);
    void prependInt64(int64_t n);
    void prependInt32(int32_t n);
    void prependInt16(int16_t n);
    void prependInt8(int8_t n);

    ssize_t readFd(int fd);
    ssize_t writeFd(int fd);

    char *writePos() { return begin() + writeIndex; }
    const char *readPos() const { return begin() + readIndex; }
private:
    const char *begin() const { return &*buffer.begin(); }
    char *begin() { return &*buffer.begin(); }
    char *readPos() { return begin() + readIndex; }
    const char *writePos() const { return begin() + writeIndex; }
    void ensureSpace(size_t len);

private:
    std::vector<char> buffer;
    size_t readIndex;
    size_t writeIndex;
    static const char CRLF[3];
};
} // namespace net
} // namespace sevent

#endif