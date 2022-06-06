#ifndef SEVENT_BASE_LOGSTREAM
#define SEVENT_BASE_LOGSTREAM

#include "sevent/base/FixedBuffer.h"
#include "sevent/base/noncopyable.h"
#include <string>

namespace sevent {

class LogStream : noncopyable {
public:
    static const int samllBufferSize = 4000; // 4KB
    using Buffer = FixedBuffer<samllBufferSize>;

    void append(const char *data, int len) { buffer.append(data, len); }
    void reset() { buffer.reset(); }
    Buffer &getBuffer() { return buffer; }

    LogStream &operator<<(bool);
    LogStream &operator<<(short);
    LogStream &operator<<(unsigned short);
    LogStream &operator<<(int);
    LogStream &operator<<(unsigned int);
    LogStream &operator<<(long);
    LogStream &operator<<(unsigned long);
    LogStream &operator<<(long long);
    LogStream &operator<<(unsigned long long);

    LogStream &operator<<(float);
    LogStream &operator<<(double);

    LogStream &operator<<(const void *);
    LogStream &operator<<(char);
    LogStream &operator<<(const char *);
    LogStream &operator<<(const unsigned char *str);
    LogStream &operator<<(const std::string &);

private:
    template<typename T>
    void formatInteger(T);
private:
    Buffer buffer;
    static const int maxNumericSize = 32;
};

class StreamItem{
public:
    StreamItem(const char *d, int l) : data(d), len(l) {}
    friend LogStream &operator<<(LogStream &stream, const StreamItem &item){
        stream.append(item.data, item.len);
        return stream;
    }
    const char *data;
    const int len;
};

} // namespace sevent

#endif