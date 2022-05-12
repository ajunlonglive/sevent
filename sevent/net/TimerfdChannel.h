#ifndef SEVENT_NET_TIMERFDCHANNEL_H
#define SEVENT_NET_TIMERFDCHANNEL_H
#ifndef _WIN32
#include "sevent/base/Timestamp.h"
#include "sevent/net/Channel.h"
#include <functional>

namespace sevent {
namespace net {
class EventLoop;

class TimerfdChannel : public Channel{
public:
    TimerfdChannel(EventLoop *loop, std::function<void()> cb);
    ~TimerfdChannel();
    void resetExpired(Timestamp expired);
private:
    void handleRead() override;
    socket_t createTimerfd();
private:
    const std::function<void()> readCallBack;
};


} // namespace net
} // namespace sevent

#endif
#endif