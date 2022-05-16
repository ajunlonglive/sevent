#ifndef SEVENT_NET_SOCKETOPS_H
#define SEVENT_NET_SOCKETOPS_H

#include "sevent/net/util.h"
#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#include <stdio.h>
#endif
namespace sevent {
namespace net {
namespace sockets {
extern const socklen_t addr6len;

int getErrno();
void setErrno(int err);
ssize_t readv(socket_t fd, const struct iovec *iov, int iovcnt);
ssize_t read(socket_t fd, void *buf, size_t count);
ssize_t write(socket_t fd, const void *buf, size_t count);
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

socket_t open(const char *pathname, int flags);
int close(socket_t fd);
int shutdown(socket_t sockfd, int how);

socket_t socket(int domain, int type, int protocol);
int bind(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(socket_t sockfd, int backlog);
socket_t accept(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen);
socket_t accept4(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
int connect(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen);

int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

int setsockopt(socket_t sockfd, int level, int optname, const void *optval, socklen_t optlen);
int getsockopt(socket_t sockfd, int level, int optname, void *optval, socklen_t *optlen);
int getsockname(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getpeername(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen);

// for Acceptor
socket_t openIdelFd();
socket_t createNBlockfd(uint16_t family);
void setReuseAddr(socket_t sockfd, bool on);
void setTcpNoDelay(socket_t sockfd, bool on);
void setKeepAlive(socket_t sockfd, bool on);
int dolisten(socket_t sockfd);
socket_t doaccept(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen);
// for TcpConnection
int getSocketError(socket_t sockfd);
void shutdownWrite(socket_t sockfd);

// for Connector
struct sockaddr_in6 getLocalAddr(socket_t sockfd);
struct sockaddr_in6 getPeerAddr(socket_t sockfd);
bool isSelfConnect(socket_t sockfd);

// for windows
void setNoBlockAndCloseonexecFast(socket_t sockfd);
void createSocketpair(socket_t fd[2]);

} // namespace sockets
} // namespace net
} // namespace sevent

#endif