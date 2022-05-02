#ifndef SEVENT_NET_POLLER_H
#define SEVENT_NET_POLLER_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include <map>
#include <vector>
namespace sevent {
namespace net {

class Channel;
class EventLoop;

class Poller : noncopyable {
public:
    Poller() = default;
    virtual ~Poller() = default;

    // poll/epoll_wait,(timeout:millsecond)必须在ownerLoop调用
    int poll(int timeout);
    virtual int doPoll(int timeout) = 0;

    // 更新/移出监听事件列表,必须在ownerLoop调用
    virtual void updateChannel(Channel *channel) = 0;

    // 移出监听事件列表和channelMap,必须在ownerLoop调用(TcpConnection析构才close(fd))
    virtual void removeChannel(Channel* channel) = 0;

    std::vector<Channel *> &getActiveChannels() { return activeChannels; }
    Timestamp getPollTime() { return pollTime; }

protected:
    virtual void fillActiveChannels(int count) = 0;
protected:
    Timestamp pollTime;
    std::vector<Channel *> activeChannels;
    std::map<int, Channel *> channelMap;
};

} // namespace net
} // namespace sevent

#endif