#include "SocketsOps.h"
#include "../base/Logger.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace sevent;
using namespace sevent::net;

const socklen_t sockets::addr6len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));

namespace {
class IgnoreSigPipe {
public:
    IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};
IgnoreSigPipe ignoreSigPipe;
} // namespace

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
        LOG_SYSFATAL << "sockets::open() faile, path = "<< pathname;
    return ret;
}
int sockets::close(int fd) {
    int ret = ::close(fd);
    if (ret < 0)
        LOG_SYSERR << "sockets::close() failed, fd = "<< fd;
    return ret;
}

int sockets::shutdown(int sockfd, int how) {
    int ret = ::shutdown(sockfd, how);
    if (ret < 0)
        LOG_SYSERR << "sockets::shutdown() failed, fd = "<< sockfd;
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
        LOG_SYSERR << "sockets::inet_pton() failed, ip = " << src;
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
int sockets::getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = ::getpeername(sockfd, addr, addrlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::getpeername() failed";
    return ret;    
}

int sockets::openIdelFd() { return sockets::open("/dev/null", O_RDONLY | O_CLOEXEC); }

int sockets::createNBlockfd(sa_family_t family) {
    return sockets::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

void sockets::setReuseAddr(int sockfd, bool on) {
    int optval = on ? 1 : 0;
    sockets::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof(optval)));
}
void sockets::setTcpNoDelay(int sockfd, bool on) {
    int optval = on ? 1 : 0;
    sockets::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof(optval)));    
}
void sockets::setKeepAlive(int sockfd, bool on) {
    int optval = on ? 1 : 0;
    sockets::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
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

void sockets::shutdownWrite(int sockfd) { sockets::shutdown(sockfd, SHUT_WR); }

struct sockaddr_in6 sockets::getLocalAddr(int sockfd) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    sockets::getsockname(sockfd, reinterpret_cast<sockaddr *>(&addr), &addrlen);
    return addr;
}
struct sockaddr_in6 sockets::getPeerAddr(int sockfd) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    sockets::getpeername(sockfd, reinterpret_cast<sockaddr *>(&addr), &addrlen);
    return addr;
}
bool sockets::isSelfConnect(int sockfd) {
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET) {
        const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
        const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port
        && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    } else if (localaddr.sin6_family == AF_INET6) {
        return localaddr.sin6_port == peeraddr.sin6_port &&
               memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr,
                      sizeof localaddr.sin6_addr) == 0;
    } else {
        return false;
    }
}