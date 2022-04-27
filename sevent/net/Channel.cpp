#include "Channel.h"
#include "../base/Logger.h"
#include "EventLoop.h"

#include <poll.h>

using namespace sevent;
using namespace sevent::net;

const int Channel::NoneEvent = 0;
const int Channel::ReadEvent = POLLIN | POLLPRI;
const int Channel::WriteEvent = POLLOUT;

Channel::Channel(int fd, EventLoop *loop)
    : fd(fd), events(0), revents(0), index(-1), ownerLoop(loop) {}

void Channel::handleEvent() {
    if ((revents & POLLHUP) && !(revents & POLLIN))
        if (closeCallback)
            closeCallback();
    if (revents & POLLNVAL)
        LOG_WARN << "Channel::handleEvent() - revents:POLLNVAL";
    if (revents & (POLLERR | POLLNVAL))
        if (errorCallback)
            errorCallback();
    if (revents & (POLLIN | POLLPRI | POLLRDHUP))
        if (readCallback)
            readCallback();
    if (revents & (POLLOUT))
        if (writeCallback)
            writeCallback();
}

void Channel::updateEvent() { ownerLoop->updateChannel(this); }
void Channel::remove() { ownerLoop->removeChannel(this); }