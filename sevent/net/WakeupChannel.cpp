#include "WakeupChannel.h"

#include "../base/Logger.h"
#include "Channel.h"
#include "SocketsOps.h"
#include <stdint.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

WakeupChannel::WakeupChannel(EventLoop *loop)
    : evfd(createEventFd()), wakeupChannel(evfd, loop) {
    // TODO 移动到EventLoop?
    wakeupChannel.setReadCallback(std::bind(&WakeupChannel::handleRead, this));
    wakeupChannel.enableReadEvent();
}

int WakeupChannel::createEventFd() {
    int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
        LOG_SYSFATAL << "WakeupChannel::createEventFd - eventfd() failed";
    return fd;
}

void WakeupChannel::wakeup() {
    uint64_t n = 1;
    ssize_t ret = sockets::write(evfd, &n, sizeof(n));
    if (ret != sizeof(n))
        LOG_ERROR << "WakeupChannel::wakeup() - write:" << ret
                  << "bytes instead of 8";
}

void WakeupChannel::handleRead() {
    uint64_t n = 1;
    ssize_t ret = sockets::read(evfd, &n, sizeof(n));
    if (ret != sizeof(n))
        LOG_ERROR << "WakeupChannel::handleRead() - read:" << ret
                  << "bytes instead of 8";    
}

WakeupChannel::~WakeupChannel() { sockets::close(evfd); }