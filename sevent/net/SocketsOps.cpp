#include "SocketsOps.h"
#include "../base/Logger.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace sevent;
using namespace sevent::net;

ssize_t sockets::readv(int fd, const struct iovec *iov, int iovcnt){
    return ::readv(fd, iov, iovcnt);
}

ssize_t sockets::read(int fd, void *buf, size_t count) {
    return ::read(fd, buf, count);
}

ssize_t sockets::write(int fd, const void *buf, size_t count){
    return ::write(fd, buf, count);
}
int sockets::close(int fd) {
    int ret = ::close(fd);
    if (ret < 0)
        LOG_SYSERR << "sockets::close() failed - fd:"<<fd;
    return ret;
}

int sockets::socket(int domain, int type, int protocol) {
    int fd = ::socket(domain, type, protocol);
    if (fd < 0)
        LOG_SYSFATAL << "sockets::socket() failed";
    return fd;
}
int sockets::bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret = ::bind(sockfd, addr, addrlen);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::bind() failed";
    return ret;
}
int sockets::listen(int sockfd, int backlog) {
    int ret = ::listen(sockfd, backlog);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::listen() failed";
    return ret;
}
int sockets::accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd = ::accept(sockfd, addr, addrlen);
    return connfd;
}
int sockets::accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    int connfd = ::accept4(sockfd, addr, addrlen, flags);
    return connfd;
}
int sockets::connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return ::connect(sockfd, addr, addrlen);
}

int sockets::inet_pton(int af, const char *src, void *dst) {
    int ret = ::inet_pton(af, src, dst);
    if (ret <= 0)
        LOG_SYSERR << "sockets::inet_pton() failed";
    return ret;
}

const char *sockets::inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    return ::inet_ntop(af, src, dst, size);
}