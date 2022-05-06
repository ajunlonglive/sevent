#ifndef SEVENT_NET_SOCKETOPS_H
#define SEVENT_NET_SOCKETOPS_H

#include <arpa/inet.h>

namespace sevent {
namespace net {
namespace sockets {
extern const socklen_t addr6len;

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

int open(const char *pathname, int flags);
int close(int fd);
int shutdown(int sockfd, int how);

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// for Acceptor
int openIdelFd();
int createNBlockfd(sa_family_t family);
void setReuseAddr(int sockfd, bool on);
void setTcpNoDelay(int sockfd, bool on);
void setKeepAlive(int sockfd, bool on);
int dolisten(int sockfd);
int doaccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// for TcpConnection
int getSocketError(int sockfd);
void shutdownWrite(int sockfd);

// for Connector
struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);

} // namespace sockets
} // namespace net
} // namespace sevent

#endif