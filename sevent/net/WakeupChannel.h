#ifndef SEVENT_NET_WAKEUPCHANNEL_H
#define SEVENT_NET_WAKEUPCHANNEL_H

#include "Channel.h"
#include <memory>
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
    int createEventFd();
    void handleRead() override;
};

} // namespace net
} // namespace sevent

#endif