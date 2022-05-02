#include "SocketsOps.h"
#include "../base/Logger.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace sevent;
using namespace sevent::net;

const socklen_t sockets::addr6len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));

ssize_t sockets::readv(int fd, const struct iovec *iov, int iovcnt){
    return ::readv(fd, iov, iovcnt);
}

ssize_t sockets::read(int fd, void *buf, size_t count) {
    return ::read(fd, buf, count);
}

ssize_t sockets::write(int fd, const void *buf, size_t count){
    return ::write(fd, buf, count);
}
int sockets::open(const char *pathname, int flags) {
    int ret = ::open(pathname, flags);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::open() faile - path:"<< pathname;
    return ret;
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
int sockets::setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    int ret = ::setsockopt(sockfd, level, optname, optval, optlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::setsockopt() failed";
    return ret;    
}
int sockets::getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    int ret = ::getsockopt(sockfd, level, optname, optval, optlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::getsockopt() failed";
    return ret;       

}
int sockets::getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = ::getsockname(sockfd, addr, addrlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::getsockname() failed";
    return ret;    
}

int sockets::openIdelFd() { return sockets::open("/dev/null", O_RDONLY | O_CLOEXEC); }

int sockets::createNBlockfd(sa_family_t family) {
    return sockets::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

void sockets::setReuseAddr(int sockfd, bool b) {
    int optval = b ? 1 : 0;
    sockets::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof(optval)));
}
int sockets::dolisten(int sockfd) { return sockets::listen(sockfd, SOMAXCONN); }
int sockets::doaccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd = sockets::accept4(sockfd, addr, addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        int err = errno;
        // LOG_SYSERR << "sockets::doaccept() err";
        switch (err) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: 
            case EPERM:
            case EMFILE:
                errno = err;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG_FATAL << "unexpected error of accept() " << err;
                break;
            default:
                LOG_FATAL << "unknown error of accept() " << err;
                break;
        }
    }
    return connfd;
}

int sockets::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    int ret = sockets::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (ret < 0)
        return errno;
    return optval;
}