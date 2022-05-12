#ifndef SEVENT_NET_EPOLLPOLLER_H
#define SEVENT_NET_EPOLLPOLLER_H

#ifndef _WIN32

#include "Poller.h"

struct epoll_event;

namespace sevent {
namespace net {

class EpollPoller : public Poller {
public:
    EpollPoller();
    ~EpollPoller() override;

    // poll/epoll_wait,必须在ownerLoop调用
    int doPoll(int timeout) override;

    void updateChannel(Channel *channel) override;

    void removeChannel(Channel *channel) override;

private:
    void fillActiveChannels(int count) override;
    void update(int operation, Channel *channel);

private:
    socket_t epfd;
    std::vector<epoll_event> eventList;

    static const int initSize = 16;
};

} // namespace net
} // namespace sevent
#endif
#endif