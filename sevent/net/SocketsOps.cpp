#include "sevent/net/SocketsOps.h"

#include "sevent/base/Logger.h"
#include <errno.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#else
#include "sevent/net/SocketsOpsWin.h"
#endif

using namespace sevent;
using namespace sevent::net;

const socklen_t sockets::addr6len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
#ifndef _WIN32
namespace {
class IgnoreSigPipe {
public:
    IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};
IgnoreSigPipe ignoreSigPipe;
} // namespace
#endif

#ifndef _WIN32
ssize_t sockets::readv(socket_t fd, const struct iovec *iov, int iovcnt){
    return ::readv(fd, iov, iovcnt);
}
#endif

int sockets::getErrno() {
    #ifndef _WIN32
    return errno;
    #else
    return h_errno;
    #endif
}
void sockets::setErrno(int err) {
    #ifndef _WIN32
    errno = err;
    #else
    WSASetLastError(err);
    #endif
}

ssize_t sockets::read(socket_t sockfd, void *buf, size_t count) {
    #ifndef _WIN32
    return ::read(sockfd, buf, count);
    #else
    return ::recv(sockfd, reinterpret_cast<char*>(buf), static_cast<int>(count), 0);
    #endif
}

ssize_t sockets::write(socket_t sockfd, const void *buf, size_t count) {
    #ifndef _WIN32
    return ::write(sockfd, buf, count);
    #else
    return ::send(sockfd, reinterpret_cast<const char*>(buf), static_cast<int>(count), 0);
    #endif
}
ssize_t sockets::sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
    #ifndef _WIN32
    return ::sendfile(out_fd, in_fd, offset, count);
    #else
    // return ::send(sockfd, reinterpret_cast<const char*>(buf), static_cast<int>(count), 0);
    return 0; // TODO
    #endif
}
socket_t sockets::open(const char *pathname, int flags) {
    #ifndef _WIN32
    int ret = ::open(pathname, flags);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::open() faile, path = "<< pathname;
    return ret;
    #else
    return -1;
    #endif
}
int sockets::close(socket_t sockfd) {
    #ifndef _WIN32
    int ret = ::close(sockfd);
    #else
    int ret = ::closesocket(sockfd);
    #endif    
    if (ret < 0)
        LOG_SYSERR << "sockets::close() failed, fd = "<< sockfd;
    return ret;

}

int sockets::shutdown(socket_t sockfd, int how) {
    int ret = ::shutdown(sockfd, how);
    if (ret < 0)
        LOG_SYSERR << "sockets::shutdown() failed, fd = "<< sockfd;
    return ret;    
}

socket_t sockets::socket(int domain, int type, int protocol) {
    socket_t fd = ::socket(domain, type, protocol);
    if (fd < 0)
        LOG_SYSERR << "sockets::socket() failed";
    return fd;
}
int sockets::bind(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret = ::bind(sockfd, addr, addrlen);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::bind() failed";
    return ret;
}
int sockets::listen(socket_t sockfd, int backlog) {
    int ret = ::listen(sockfd, backlog);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::listen() failed";
    return ret;
}
socket_t sockets::accept(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    socket_t connfd = ::accept(sockfd, addr, addrlen);
    return connfd;
}
#ifdef HAVE_ACCEPT4
socket_t sockets::accept4(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    socket_t connfd = ::accept4(sockfd, addr, addrlen, flags);
    return connfd;
}
#endif
int sockets::connect(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return ::connect(sockfd, addr, addrlen);
}

int sockets::inet_pton(int af, const char *src, void *dst) {
    #ifndef _WIN32
    int ret = ::inet_pton(af, src, dst);
    #else
    int ret = sockets::evutil_inet_pton(af, src, dst);
    #endif
    if (ret <= 0)
        LOG_SYSERR << "sockets::inet_pton() failed, ip = " << src;
    return ret;
}

const char *sockets::inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    #ifndef _WIN32
    return ::inet_ntop(af, src, dst, size);
    #else
    return sockets::evutil_inet_ntop(af, src, dst, size);
    #endif
}
int sockets::setsockopt(socket_t sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    int ret = ::setsockopt(sockfd, level, optname, reinterpret_cast<const char *>(optval), optlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::setsockopt() failed, fd = " << sockfd
                   << " ,level = " << level << " ,optname = " << optname;
    return ret;
}
int sockets::getsockopt(socket_t sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    int ret = ::getsockopt(sockfd, level, optname, reinterpret_cast<char *>(optval), optlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::getsockopt() failed, fd = " << sockfd
                   << " ,level = " << level << " ,optname = " << optname;
    return ret;       

}
int sockets::getsockname(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = ::getsockname(sockfd, addr, addrlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::getsockname() failed, fd = " << sockfd;
    return ret;    
}
int sockets::getpeername(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = ::getpeername(sockfd, addr, addrlen);
    if (ret < 0)
        LOG_SYSERR << "sockets::getpeername() failed, fd = " << sockfd;
    return ret;    
}

socket_t sockets::openIdelFd() {
    #ifndef _WIN32
    return sockets::open("/dev/null", O_RDONLY | O_CLOEXEC); 
    #else
    return -1;
    #endif
}

socket_t sockets::createNBlockfd(uint16_t family) {
    #ifndef _WIN32
    return sockets::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    #else
    socket_t sockfd = sockets::socket(family, SOCK_STREAM, IPPROTO_TCP);
    sockets::setNoBlockAndCloseonexecFast(sockfd);
    return sockfd;
    #endif
}

void sockets::setReuseAddr(socket_t sockfd, bool on) {
    int optval = on ? 1 : 0;
    sockets::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof(optval)));
}
void sockets::setTcpNoDelay(socket_t sockfd, bool on) {
    int optval = on ? 1 : 0;
    sockets::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof(optval)));    
}
void sockets::setKeepAlive(socket_t sockfd, bool on) {
    int optval = on ? 1 : 0;
    sockets::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof(optval)));
}
int sockets::dolisten(socket_t sockfd) { return sockets::listen(sockfd, SOMAXCONN); }
socket_t sockets::doaccept(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    #ifdef HAVE_ACCEPT4
    socket_t connfd = sockets::accept4(sockfd, addr, addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    #else
    socket_t connfd = sockets::accept(sockfd, addr, addrlen);
    if (connfd >= 0)
        sockets::setNoBlockAndCloseonexecFast(connfd);
    #endif
    if (connfd < 0) {
        int err = getErrno();
        switch (err) {
            #ifndef _WIN32
            case EWOULDBLOCK: //linux: EAGAIN = EWOULDBLOCK
                errno = err;
                break;
            case ECONNABORTED:
            case EINTR:
            case EPROTO: 
            case EPERM:
            case EMFILE:
            #else
            case WSAEWOULDBLOCK:
                sockets::setErrno(err);
                break;
            case WSAECONNABORTED:
            case WSAEINTR:
            case WSAEPROTOTYPE: 
            case WSAEACCES:
            case WSAEMFILE:
                sockets::setErrno(err);
            #endif
                errno = err;
                LOG_SYSERR << "expected error of accept()";
                break;
            #ifndef _WIN32
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
            #else
            case WSAEBADF:
            case WSAEFAULT:
            case WSAEINVAL:
            case WSAENOBUFS:
            case WSA_NOT_ENOUGH_MEMORY:
            case WSAENOTSOCK:
            case WSAEOPNOTSUPP:
            #endif
                // unexpected errors
                LOG_SYSERR << "unexpected error of accept()";
                break;
            default:
                LOG_SYSERR << "other error of accept()";
                break;
        }
    }
    return connfd;
}

int sockets::getSocketError(socket_t sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
    int ret = sockets::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (ret < 0)
        #ifndef _WIN32
        return errno;
        #else
        return h_errno;
        #endif
    return optval;
}

void sockets::shutdownWrite(socket_t sockfd) {
    #ifndef _WIN32
    sockets::shutdown(sockfd, SHUT_WR); 
    #else
    sockets::shutdown(sockfd, SD_SEND); 
    #endif
}

struct sockaddr_in6 sockets::getLocalAddr(socket_t sockfd) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    sockets::getsockname(sockfd, reinterpret_cast<sockaddr *>(&addr), &addrlen);
    return addr;
}
struct sockaddr_in6 sockets::getPeerAddr(socket_t sockfd) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    sockets::getpeername(sockfd, reinterpret_cast<sockaddr *>(&addr), &addrlen);
    return addr;
}
bool sockets::isSelfConnect(socket_t sockfd) {
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

void sockets::setNoBlockAndCloseonexecFast(socket_t sockfd) {
    #ifdef _WIN32
    unsigned long nonblocking = 1;
    if (ioctlsocket(sockfd, FIONBIO, &nonblocking) == SOCKET_ERROR) 
        LOG_SYSERR << "sockets::setNoBlockAndCloseonexecFast() - FIONBIO failed, fd = "<< sockfd;
    #else
    if (::fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
        LOG_SYSERR << "sockets::setNoBlockAndCloseonexecFast() - O_NONBLOCK failed, fd = "<< sockfd;
    if (::fcntl(sockfd, F_SETFD, FD_CLOEXEC) == -1)
        LOG_SYSERR << "sockets::setNoBlockAndCloseonexecFast() - FD_CLOEXEC failed, fd = "<< sockfd;
    #endif
}

void sockets::createSocketpair(socket_t fd[2]) {
    #ifndef _WIN32
    int ret = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if (ret < 0)
        LOG_SYSFATAL << "sockets::createSocketpair() - socketpair failed";
    #else
    socket_t acceptor = -1;
    socket_t listener = sockets::socket(AF_INET, SOCK_STREAM, 0);
    socket_t connector = sockets::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in listenAddr;
    struct sockaddr_in connectAddr;
    socklen_t len = sizeof(listenAddr);
    memset(&listenAddr, 0, sizeof(listenAddr));
    memset(&connectAddr, 0, sizeof(connectAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    listenAddr.sin_port = 0; // 系统选择
    sockets::bind(listener, reinterpret_cast<struct sockaddr *>(&listenAddr), len);
    sockets::listen(listener, 1);
    // 找到listener端口
    if (sockets::getsockname(listener, reinterpret_cast<struct sockaddr *>(&connectAddr), &len) == -1)
        LOG_SYSFATAL << "sockets::createSocketpair() - getsockname failed";
    if (sockets::connect(connector, reinterpret_cast<struct sockaddr *>(&connectAddr), len) == -1)
        LOG_SYSFATAL << "sockets::createSocketpair() - connect failed";
    acceptor = sockets::accept(listener, reinterpret_cast<struct sockaddr *>(&listenAddr), &len);
    if (acceptor < 0)
        LOG_SYSFATAL << "sockets::createSocketpair() - accept failed";
    if (sockets::getsockname(connector, reinterpret_cast<struct sockaddr *>(&connectAddr), &len) == -1)
        LOG_SYSFATAL << "sockets::createSocketpair() - getsockname failed";
    if (len != sizeof(listenAddr) ||
        listenAddr.sin_family != connectAddr.sin_family ||
        listenAddr.sin_addr.s_addr != connectAddr.sin_addr.s_addr ||
        listenAddr.sin_port != connectAddr.sin_port)
        LOG_SYSFATAL << "sockets::createSocketpair() - listenAddr != connectAddr";
    sockets::close(listener);
    fd[0] = connector;
    fd[1] = acceptor;
    setNoBlockAndCloseonexecFast(fd[0]);
    setNoBlockAndCloseonexecFast(fd[1]);
#endif
}