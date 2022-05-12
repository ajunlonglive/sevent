#include "sevent/net/TimerfdChannel.h"
#ifndef _WIN32
#include "sevent/base/Logger.h"
#include "sevent/net/SocketsOps.h"
#include <sys/timerfd.h>
using namespace std;
using namespace sevent;
using namespace sevent::net;

TimerfdChannel::TimerfdChannel(EventLoop *loop, function<void()> cb)
    : Channel(createTimerfd(), loop), readCallBack(std::move(cb)) {
    Channel::enableReadEvent();
}

TimerfdChannel::~TimerfdChannel() {
    Channel::remove();
    sockets::close(fd);
}

socket_t TimerfdChannel::createTimerfd() {
    socket_t fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0)
        LOG_SYSFATAL << "timerfd_create() failed";
    return fd;
}

void TimerfdChannel::resetExpired(Timestamp expired) {
    struct itimerspec newValue;
    memset(&newValue, 0, sizeof(newValue));
    int64_t relTime =
        expired.getMicroSecond() - Timestamp::now().getMicroSecond();
    if (relTime < 100)
        relTime = 100; // microsecond
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(relTime / Timestamp::microSecondUnit);
    ts.tv_nsec =
        static_cast<long>((relTime % Timestamp::microSecondUnit)) * 1000;
    newValue.it_value = ts;
    int ret = timerfd_settime(fd, 0, &newValue, nullptr);
    if (ret)
        LOG_SYSERR << "timerfd_settime() failed";
}

void TimerfdChannel::handleRead() {
    uint64_t t;
    ssize_t ret = sockets::read(fd, &t, sizeof(t));
    if (ret != sizeof(t))
        LOG_ERROR << "TimerManager::readTimerfd() - read:" << ret
                  << "bytes instead of 8";
    if (readCallBack)
        readCallBack();
}
#endif