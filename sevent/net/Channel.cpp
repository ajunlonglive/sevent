#include "sevent/net/Channel.h"

#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#ifndef _WIN32
#include <poll.h>
#else
namespace {
const int POLLIN = 0x001;
const int POLLPRI = 0x002;
const int POLLOUT = 0x004;
const int POLLERR = 0x008;
const int POLLHUP = 0x010;
const int POLLNVAL = 0x020;
const int POLLRDHUP = 0x2000;
}
#endif

using namespace sevent;
using namespace sevent::net;

const int Channel::NoneEvent = 0;
const int Channel::ReadEvent = POLLIN | POLLPRI;
const int Channel::WriteEvent = POLLOUT;

Channel::Channel(socket_t fd, EventLoop *loop)
    : fd(fd), events(0), revents(0), index(-1), ownerLoop(loop) {}

void Channel::handleEvent() {
    ownerLoop->assertInOwnerThread();
    if ((revents & POLLHUP) && !(revents & POLLIN))
        handleClose();
    if (revents & POLLNVAL)
        LOG_WARN << "Channel::handleEvent() - revents:POLLNVAL";
    if (revents & (POLLERR | POLLNVAL))
        handleError();
    if (revents & (POLLIN | POLLPRI | POLLRDHUP))
        handleRead();
    if (revents & (POLLOUT))
        handleWrite();
}

void Channel::updateEvent() { ownerLoop->updateChannel(this); }
void Channel::remove() { 
    events = NoneEvent;
    ownerLoop->removeChannel(this); 
}
void Channel::setFd(socket_t sockfd) {
    remove();
    fd = sockfd;
}