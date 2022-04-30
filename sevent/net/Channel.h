#ifndef SEVENT_NET_CHANNEL_H
#define SEVENT_NET_CHANNEL_H

#include "../base/noncopyable.h"
#include <functional>

namespace sevent {
namespace net {
class EventLoop;
// Channel 是fd的抽象,EventLoop通过Poller间接管理Channel
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    
    Channel(int fd, EventLoop *loop);

    void handleEvent();
    void setRevents(int revt) { revents = revt; }
    bool isNoneEvent() const { return events == NoneEvent; }
    // 更新/移出监听事件列表(Poller)
    void enableReadEvent() { events |= ReadEvent; updateEvent(); }
    void enablewriteEvent() { events |= WriteEvent; updateEvent(); }
    void disableReadEvent() { events &= ~ReadEvent; updateEvent(); }
    void disablewriteEvent() { events &= ~WriteEvent; updateEvent(); }
    void disableAll() { events = NoneEvent; updateEvent(); }
    // 移出监听事件列表和channelMap(Poller)
    void remove();

    int getFd() const { return fd; }
    int getEvents() const { return events; }
    EventLoop *getOwnerLoop() { return ownerLoop; }

    int getIndex() { return index; }    //for PollPoller
    int getStatus() { return index; }   //for EPollPoller
    void setIndex(int i) { index = i; }
    void setStatus(int i) { index = i; }

private:
    void updateEvent();
protected:
    // for override
    virtual void handleRead() {}
    virtual void handleWrite() {}
    virtual void handleClose() {}
    virtual void handleError() {}

protected:
    int fd;
    int events;
    int revents;
    int index;  //PollPoller::pollfdList[index] //EpollPoller,status
    EventLoop *ownerLoop;
public:
    static const int NoneEvent;  //0
    static const int ReadEvent;  //POLLIN | POLLPRI;
    static const int WriteEvent; //POLLOUT
};

} // namespace net
} // namespace sevent

#endif