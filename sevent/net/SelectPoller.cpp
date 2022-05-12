#include "sevent/net/SelectPoller.h"

#include "sevent/net/Channel.h"
#include <assert.h>
#include <time.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif
using namespace std;
using namespace sevent;
using namespace sevent::net;
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
        socket_t fd = ch->getFd();
        bool isread = false;
        bool iswrite = false;
        if (ch->getEvents() & Channel::ReadEvent) {
            FD_SET(fd, &rset);
            FD_SET(fd, &eset);
            isread = true;
        }
        if (ch->getEvents() & Channel::WriteEvent) {
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
if (channel->getIndex() < 0) {
        //新增
        assert(channelMap.find(channel->getFd()) == channelMap.end());
        channelMap[channel->getFd()] = channel;
    } else {
        assert(channelMap.find(channel->getFd()) != channelMap.end());
        assert(channelMap[channel->getFd()] == channel);
    }
}

// 移除 channelMap[fd]
void SelectPoller::removeChannel(Channel* channel) {
    channelMap.erase(channel->getFd());
}


void SelectPoller::fillActiveChannels(int count) {
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