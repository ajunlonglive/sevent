#ifndef SEVENT_NET_SOCKETOPS_H
#define SEVENT_NET_SOCKETOPS_H

#include <arpa/inet.h>

namespace sevent {
namespace net {
namespace sockets {

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

} // namespace sockets
} // namespace net
} // namespace sevent

#endif