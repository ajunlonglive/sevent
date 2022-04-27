#ifndef SEVENT_NET_SELECTPOLLER_H
#define SEVENT_NET_SELECTPOLLER_H

#include "Poller.h"


namespace sevent {
namespace net {
class Channel;

class SelectPoller : public Poller {
public:
    SelectPoller();
    ~SelectPoller() override;

    // poll/epoll_wait
    int doPoll(int timeout) override;

    // 更新事件到poll的监听队列
    void updateChannel(Channel *channel) override;

    // 移除监听队列
    void removeChannel(Channel* channel) override;


private:
    void fillActiveChannels(int count) override;
    void reset();

private:
    int maxfd;
    fd_set rset;
    fd_set wset;
    fd_set eset;
};


} // namespace net
} // namespace sevent

#endif