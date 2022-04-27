#ifndef SEVENT_NET_POLLPOLLER_H
#define SEVENT_NET_POLLPOLLER_H

#include "Poller.h"

struct pollfd;

namespace sevent {
namespace net {
class Channel;

class PollPoller : public Poller {
public:
    PollPoller();
    ~PollPoller() override;

    // poll/epoll_wait
    int doPoll(int timeout) override;

    // 更新/移出监听事件列表,必须在ownerLoop调用
    void updateChannel(Channel *channel) override;

    // 移出监听事件列表和channelMap,必须在ownerLoop调用
    void removeChannel(Channel *channel) override;

private:
    void fillActiveChannels(int count) override;

private:
    std::vector<pollfd> pollfdList;
};

} // namespace net
} // namespace sevent

#endif