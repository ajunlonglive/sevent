#ifndef SEVENT_NET_WAKEUPCHANNEL_H
#define SEVENT_NET_WAKEUPCHANNEL_H

#include "sevent/net/Channel.h"
namespace sevent {
namespace net {
class Channel;
class EventLoop;

class WakeupChannel : public Channel {
public:
    WakeupChannel(EventLoop *loop);
    ~WakeupChannel();
    void wakeup();

private:
    socket_t createEventFd();
    void handleRead() override;
private:
#ifdef _WIN32
    socket_t fds[2];
#endif
};

} // namespace net
} // namespace sevent

#endif