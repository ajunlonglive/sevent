#ifndef SEVENT_NET_WAKEUPCHANNEL_H
#define SEVENT_NET_WAKEUPCHANNEL_H

#include "Channel.h"
#include "../base/noncopyable.h"
#include <memory>
namespace sevent {
namespace net {
class Channel;
class EventLoop;

class WakeupChannel : noncopyable {
public:
    WakeupChannel(EventLoop *loop);
    ~WakeupChannel();
    void wakeup();

private:
    int createEventFd();
    void handleRead();

private:
    int evfd;
    Channel wakeupChannel;
};

} // namespace net
} // namespace sevent

#endif