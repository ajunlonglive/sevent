#include "sevent/net/PollPoller.h"
#ifndef _WIN32
#include "sevent/net/Channel.h"
#include <assert.h>
#include <poll.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

PollPoller::PollPoller() = default;
PollPoller::~PollPoller() = default;

int PollPoller::doPoll(int timeout){
    int count = ::poll(&*pollfdList.begin(), pollfdList.size(), timeout);
    return count;
}
void PollPoller::fillActiveChannels(int count){
    using iter = vector<pollfd>::iterator;
    for (iter it = pollfdList.begin(); it != pollfdList.end() && count > 0; ++it) {
        if (it->revents <= 0)
            continue;
        --count;
        map<int, Channel *>::iterator chIter = channelMap.find(it->fd);
        assert(chIter != channelMap.end());
        Channel *channel = chIter->second;
        channel->setRevents(it->revents);
        activeChannels.push_back(channel);
    }
}
void PollPoller::updateChannel(Channel *channel){
    // ownerLoop->assertInOwnerThread();
    if (channel->getIndex() < 0) {
        //新增,添加到pollfdList
        assert(channelMap.find(channel->getFd()) == channelMap.end());
        struct pollfd pfd;
        pfd.fd = channel->getFd();
        pfd.events = static_cast<short>(channel->getEvents());
        pollfdList.push_back(pfd);
        channel->setIndex(static_cast<int>(pollfdList.size() - 1));
        channelMap[pfd.fd] = channel;
    } else {
        //更新/忽略,不关心事件的fd = -fd - 1(poll会忽略);
        //若只设置 events=0,仍会POLLHUP,POLLERR和POLLNVAL
        assert(channelMap.find(channel->getFd()) != channelMap.end());
        assert(channelMap[channel->getFd()] == channel);
        int index = channel->getIndex();
        assert(0 <= index && index < static_cast<int>(pollfdList.size()));
        struct pollfd &pfd = pollfdList[index];
        assert(pfd.fd == channel->getFd() || pfd.fd == -channel->getFd() - 1);
        pfd.fd = channel->getFd();
        pfd.events = static_cast<short>(channel->getEvents());
        pfd.revents = 0;
        if (channel->isNoneEvent())
            pfd.fd = -channel->getFd() - 1;
    }
}

void PollPoller::removeChannel(Channel* channel) {
    // ownerLoop->assertInOwnerThread();
    // assert(channelMap.find(channel->getFd()) != channelMap.end());
    // assert(channelMap[channel->getFd()] == channel);
    int index = channel->getIndex();
    if (index < 0)
        return;
    // assert(0 <= index && index < static_cast<int>(pollfdList.size()));
    struct pollfd &pfd = pollfdList[index];
    (void)pfd;
    assert(pfd.fd == channel->getFd() || pfd.fd == -channel->getFd() - 1);
    // 移除channelMap 和 pollfdList
    channelMap.erase(channel->getFd());
    if (static_cast<size_t>(index) == pollfdList.size() - 1) {
        pollfdList.pop_back();
    } else {
        // 与末尾交换,并且更新channel的index
        std::swap(pollfdList[index], pollfdList[pollfdList.size() - 1]);
        channel->setIndex(index);
        pollfdList.pop_back();
    }
}

#endif