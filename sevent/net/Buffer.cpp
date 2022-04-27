#include "Buffer.h"
#include "EndianOps.h"
#include "SocketsOps.h"
#include <assert.h>
#include <string.h>
#include <cmath>
#include <algorithm>

using std::string;
using namespace sevent;
using namespace sevent::net;

const char Buffer::CRLF[3] = "\r\n";

ssize_t Buffer::readFd(int fd){
    char extrabuf[65535]; //64KB
    size_t writeable = writableBytes();
    iovec iov[2];
    iov[0].iov_base = writePos();
    iov[0].iov_len = writeable;
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);
    //当可写空间大于额外缓存,不使用额外空间
    int iovcnt = writeable < sizeof(extrabuf) ? 2 : 1;
    ssize_t n = sockets::readv(fd, iov, iovcnt);
    if (n > 0) {
        if (static_cast<size_t>(n) <= writeable){
            writeIndex += n;
        } else{
            append(extrabuf, n - writeable);
        }
    }
    return n;
}
ssize_t Buffer::writeFd(int fd){
    ssize_t n = sockets::write(fd, readPos(), readableBytes());
    if (n > 0) 
        readIndex += n;
    return n;
}
void Buffer::retrieve(size_t len){
    if (len < readableBytes()) {
        readIndex += len;
    } else {
        retrieveAll();
    }
}
void Buffer::retrieveAll(){
    readIndex = prepends;
    writeIndex = prepends;

}
void Buffer::swap(Buffer &buf){
    buffer.swap(buf.buffer);
    std::swap(readIndex, buf.readIndex);
    std::swap(writeIndex, buf.writeIndex);
}
void Buffer::shrinkToFit(){
    Buffer buf;
    buf.ensureSpace(readableBytes());
    buf.append(readPos(), readableBytes());
    swap(buf);
    // size_t readable = readableBytes();
    // std::copy(readPos(), writePos(), begin() + Buffer::prepends);
    // readIndex = prepends;
    // writeIndex = prepends + readable;
    // buffer.resize(prepends + readable);
}
void Buffer::append(const void *data, size_t len){
    ensureSpace(len);
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, writePos());
    writeIndex += len;
}
void Buffer::append(const std::string &str) { append(str.c_str(), str.size()); }
void Buffer::prepend(const void *data, size_t len){
    size_t prependable = prependableBytes();
    len = len > prependable ? prependable : len;
    readIndex -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, readPos());
}

void Buffer::ensureSpace(size_t len){
    if (writableBytes() >= len)
        return;
    //(size - writeIndex) + readIndex - prepends;扩容或移动
    if (writableBytes() + prependableBytes() - prepends < len) {
        buffer.resize(writeIndex + len);
    } else {
        size_t readable = readableBytes();
        std::copy(readPos(), writePos(), buffer.begin() + prepends);
        readIndex = prepends;
        writeIndex = prepends + readable;
    }
    assert(writableBytes() >= len);
}

size_t Buffer::readBytes(char *buf, size_t len){
    size_t readable = readableBytes();
    len = len > readable ? readable : len;
    std::copy(readPos(), readPos() + len, buf);
    retrieve(len);
    return len;
}
string Buffer::readAsString(size_t len){
    if (len > readableBytes()) {
        return readAllAsString();
    } else {
        string str(peek(), len);
        retrieve(len);
        return str;
    }
}
string Buffer::readAllAsString(){
    string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

const char * Buffer::findEOL() const {
    const void *pos = memchr(peek(),'\n',readableBytes());
    return static_cast<const char *>(pos);
}
const char * Buffer::findEOL(const char *start) const {
    if (start < readPos() || start > writePos())
        return nullptr;
    const void *pos = memchr(start, '\n', writePos() - start);
    return static_cast<const char *>(pos);
}

const char *Buffer::findCRLF() const{
    return findCRLF(readPos());
}
const char *Buffer::findCRLF(const char *start) const{
    if (start < readPos() || start > writePos())
        return nullptr;
    const char* crlf = std::search(start, writePos(), CRLF, CRLF+2);
    return crlf == writePos() ? nullptr : crlf;
}

int64_t Buffer::peekInt64() const {
    int64_t n = 0;
    memcpy(&n, peek(), sizeof(n));
    return sockets::netToHost64(n);
}
int32_t Buffer::peekInt32() const {
    int32_t n = 0;
    memcpy(&n, peek(), sizeof(n));
    return sockets::netToHost32(n);
}
int16_t Buffer::peekInt16() const {
    int16_t n = 0;
    memcpy(&n, peek(), sizeof(n));
    return sockets::netToHost16(n);

}
int8_t  Buffer::peekInt8() const {
    int8_t n = 0;
    memcpy(&n, peek(), sizeof(n));
    return n;
}
int64_t Buffer::readInt64(){
    int64_t n = peekInt64();
    retrieve(sizeof(n));
    return n;
}
int32_t Buffer::readInt32(){
    int32_t n = peekInt32();
    retrieve(sizeof(n));
    return n;
}
int16_t Buffer::readInt16(){
    int16_t n = peekInt16();
    retrieve(sizeof(n));
    return n;

}
int8_t  Buffer::readInt8(){
    int8_t n = peekInt8();
    retrieve(sizeof(n));
    return n;
}
void Buffer::appendInt64(int64_t n){
    int64_t num = sockets::hostToNet64(n);
    append(&num, sizeof(num));
}
void Buffer::appendInt32(int32_t n){
    int32_t num = sockets::hostToNet32(n);
    append(&num, sizeof(num));

}
void Buffer::appendInt16(int16_t n){
    int16_t num = sockets::hostToNet16(n);
    append(&num, sizeof(num));
}
void Buffer::appendInt8(int8_t n){
    append(&n, sizeof(n));
}
void Buffer::prependInt64(int64_t n){
    int64_t num = sockets::hostToNet64(n);
    prepend(&num, sizeof(num));
}
void Buffer::prependInt32(int32_t n){
    int32_t num = sockets::hostToNet32(n);
    prepend(&num, sizeof(num));
}
void Buffer::prependInt16(int16_t n){
    int16_t num = sockets::hostToNet16(n);
    prepend(&num, sizeof(num));
}
void Buffer::prependInt8(int8_t n){
    prepend(&n, sizeof(n));
}

