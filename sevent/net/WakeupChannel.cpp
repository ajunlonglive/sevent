#include "sevent/net/WakeupChannel.h"

#include "sevent/base/Logger.h"
#include "sevent/net/Channel.h"
#include "sevent/net/SocketsOps.h"
#include <stdint.h>
#ifndef _WIN32
#include <sys/eventfd.h>
#include <unistd.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

WakeupChannel::WakeupChannel(EventLoop *loop) : Channel(createEventFd(), loop) {
    Channel::enableReadEvent();
    // #ifndef _WIN32
    // LOG_TRACE << "WakeupChannel::WakeupChannel(), fd = " << fd;
    // #else
    // LOG_TRACE << "WakeupChannel::WakeupChannel(), fds[0] = " << fds[0]
    //           << ", fds[1] = " << fds[1];
    // #endif
}

socket_t WakeupChannel::createEventFd() {
    #ifndef _WIN32
    socket_t fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
        LOG_SYSFATAL << "WakeupChannel::createEventFd - eventfd() failed";
    return fd;
    #else
    sockets::createSocketpair(fds);
    return fds[0];
    #endif
}

void WakeupChannel::wakeup() {
    #ifndef _WIN32
    uint64_t n = 1;
    ssize_t ret = sockets::write(fd, &n, sizeof(n));
    if (ret != sizeof(n))
        LOG_ERROR << "WakeupChannel::wakeup() - write:" << ret
                  << "bytes instead of 8";
    #else
    ssize_t ret = sockets::write(fds[1], "a", 1);
    if (ret != 1)
        LOG_ERROR << "WakeupChannel::wakeup() - write:" << ret
                  << "bytes instead of 1";
    #endif
}

void WakeupChannel::handleRead() {
    #ifndef _WIN32
    uint64_t n = 1;
    ssize_t ret = sockets::read(fd, &n, sizeof(n));
    if (ret != sizeof(n))
        LOG_ERROR << "WakeupChannel::handleRead() - read:" << ret
                  << "bytes instead of 8";    
    #else
    char buf[256];
    while (sockets::read(fds[0], buf, sizeof(buf)) > 0)
        ;
    #endif
}

WakeupChannel::~WakeupChannel() {
    Channel::remove();
    #ifndef _WIN32
    sockets::close(fd);
    #else
    sockets::close(fds[0]);
    sockets::close(fds[1]);
    #endif
}