#include "sevent/net/poller/SelectPoller.h"

#include "sevent/net/Channel.h"
#include <assert.h>
#include <time.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif
using namespace std;
using namespace sevent;
using namespace sevent::net;
namespace {
const int ready = -1;   //既不在监听列表,也不再channelMap
const int normal = 1;   //在监听列表和channelMap
const int removed = 2;  //不在监听列表,在channelMap
} // namespace
SelectPoller::SelectPoller() : maxfd(-1) {}
SelectPoller::~SelectPoller() = default;

// static const int NoneEvent;  //0
// static const int ReadEvent;  //POLLIN | POLLPRI;
// static const int WriteEvent; //POLLOUT
void SelectPoller::reset() {
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);
    maxfd = -1;
    int count = 0;
    for (pair<const socket_t, Channel *> &item : channelMap) {
        Channel *ch = item.second;
        if (ch->getStatus() != normal)
            continue;
        socket_t fd = ch->getFd();
        bool isread = false;
        bool iswrite = false;
        if (ch->isEnableRead()) {
            FD_SET(fd, &rset);
            FD_SET(fd, &eset);
            isread = true;
        }
        if (ch->isEnableWrite()) {
            FD_SET(fd, &wset);
            iswrite = true;
        }
        
        if (isread || iswrite) {
            #ifndef _WIN32
            maxfd = fd > maxfd ? fd : maxfd;
            #endif
            if (++count >= FD_SETSIZE)
                break;
        }
    }
}
int SelectPoller::doPoll(int timeout) {
    reset();
    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(timeout / 1000);
    #ifndef _WIN32
    tv.tv_usec = static_cast<suseconds_t>((timeout % 1000)) * 1000;
    #else
    tv.tv_usec = ((timeout % 1000)) * 1000;
    #endif
    int count = select(maxfd + 1, &rset, &wset, &eset, &tv);
    return count;
}

// 更新事件到监听队列
void SelectPoller::updateChannel(Channel *channel) {
    int status = channel->getStatus();
    if (status == ready) {
        //新增
        assert(channelMap.find(channel->getFd()) == channelMap.end());
        channelMap[channel->getFd()] = channel;
        channel->setStatus(normal);
    } else if (status == normal) {
        assert(channelMap.find(channel->getFd()) != channelMap.end());
        assert(channelMap[channel->getFd()] == channel);
        if (channel->isNoneEvent())
            channel->setStatus(removed);
    } else {
        // status = removed
        channel->setStatus(normal);
    }
}

// 移除 channelMap[fd]
void SelectPoller::removeChannel(Channel* channel) {
    channelMap.erase(channel->getFd());
    channel->setStatus(ready);
}


void SelectPoller::fillActiveChannels(int count) {
    // libevent:win32select.c 是分别遍历read/write/exceptionSet
    using iter = map<socket_t, Channel *>::iterator;
    for (iter it = channelMap.begin(); it != channelMap.end() && count > 0; ++it) {
        Channel *ch = it->second;
        socket_t curfd = ch->getFd();
        bool isread = false;
        bool iswrite = false;
        if (FD_ISSET(curfd, &rset) || FD_ISSET(curfd, &eset)) {
            ch->setRevents(Channel::ReadEvent);
            isread = true;
        }
        if (FD_ISSET(curfd, &wset)) {
            ch->setRevents(isread ? Channel::WriteEvent | Channel::ReadEvent : Channel::WriteEvent);
            iswrite = true;
        }
        if (isread || iswrite) {
            activeChannels.push_back(ch);
            --count;
        }
    }
}