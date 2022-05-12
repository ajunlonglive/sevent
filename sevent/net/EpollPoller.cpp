#include "EpollPoller.h"
#ifndef _WIN32
#include "sevent/base/Logger.h"
#include "sevent/net/Channel.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/SocketsOps.h"
#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

namespace {
const int ready = -1;   //既不在监听列表,也不再channelMap
const int normal = 1;   //在监听列表和channelMap
const int removed = 2;  //不在监听列表,在channelMap
} // namespace

EpollPoller::EpollPoller()
    : epfd(::epoll_create1(EPOLL_CLOEXEC)), eventList(initSize) {
    if (epfd < 0)
        LOG_SYSFATAL << "EpollPoller::EpollPoller() - epoll_create1 err";
}
EpollPoller::~EpollPoller() { sockets::close(epfd); };

int EpollPoller::doPoll(int timeout) {
    int count = ::epoll_wait(epfd, &*eventList.begin(),
                             static_cast<int>(eventList.size()), timeout);
    return count;
}

void EpollPoller::updateChannel(Channel *channel) {
    // ownerLoop->assertInOwnerThread();
    int status = channel->getStatus();
    socket_t fd = channel->getFd();
    if (status == removed)
        return;
    if (status == ready) {
        // 新增
        assert(channelMap.find(fd) == channelMap.end());
        channelMap[fd] = channel;
        channel->setStatus(normal);
        update(EPOLL_CTL_ADD, channel);
    } else {
        // 更新/忽略
        assert(channelMap.find(fd) != channelMap.end());
        assert(channelMap.find(fd)->second == channel);
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setStatus(removed);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel *channel) {
    // ownerLoop->assertInOwnerThread();
    socket_t fd = channel->getFd();
    // assert(channelMap.find(fd) != channelMap.end());
    // assert(channelMap.find(fd)->second == channel);
    channelMap.erase(fd);
    if (channel->getStatus() == normal)
        update(EPOLL_CTL_DEL, channel);
    channel->setStatus(ready);
}

void EpollPoller::update(int op, Channel *channel) {
    struct epoll_event event;
    event.data.ptr = channel;
    event.events = channel->getEvents();
    socket_t fd = channel->getFd();
    int ret = ::epoll_ctl(epfd, op, fd, &event);
    if (ret < 0) {
        if (op == EPOLL_CTL_DEL)
            LOG_SYSERR << "EPOLL_CTL_DEL fd = " << fd;
        else if (op == EPOLL_CTL_ADD)
            LOG_SYSFATAL << "EPOLL_CTL_ADD fd = " << fd;
        else if (op == EPOLL_CTL_MOD)
            LOG_SYSFATAL << "EPOLL_CTL_MOD fd = " << fd;
        else
            LOG_SYSFATAL << "UNKONW fd = " << fd;
    }
}

void EpollPoller::fillActiveChannels(int count) {
    for (int i = 0; i < count; ++i) {
        Channel *channel = static_cast<Channel *>(eventList[i].data.ptr);
        channel->setRevents(eventList[i].events);
        activeChannels.push_back(channel);
    }
    if (static_cast<size_t>(count) == eventList.size())
        eventList.resize(eventList.size() * 2);
}

#endif