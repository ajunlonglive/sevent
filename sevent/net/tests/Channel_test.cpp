#ifndef _WIN32
#include "sevent/net/Channel.h"
#include "sevent/net/EventLoop.h"
#include "sevent/base/Timestamp.h"
#include "sevent/base/CurrentThread.h"
#include <assert.h>
#include <functional>
#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

class PeriodicTimer : Channel {
public:
    PeriodicTimer(EventLoop *loop, double interval, function<void()> cb)
        : Channel(createTimerfd(), loop), interval(interval),timercallback(std::move(cb)) {
        Channel::enableReadEvent();
    }
    ~PeriodicTimer() {
        Channel::remove();
        ::close(fd);
    }
    void start() {
        struct itimerspec spec;
        memset(&spec, 0, sizeof spec);
        spec.it_interval = toTimeSpec(interval);
        spec.it_value = spec.it_interval;
        int ret = ::timerfd_settime(fd, 0 /* relative timer */, &spec, NULL);
        assert(ret == 0);
        (void)ret;
    }

private:
    void handleRead() override {
        ownerLoop->assertInOwnerThread();
        readTimerfd();
        if (timercallback)
            timercallback();
    }
    int createTimerfd() {
        int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        assert(fd >= 0);
        return fd;
    }
    void readTimerfd() {
        uint64_t t;
        ssize_t ret = read(fd, &t, sizeof(t));
        assert(ret == sizeof t);
        (void)ret;
    }
    static struct timespec toTimeSpec(double seconds) {
        struct timespec ts;
        memset(&ts, 0, sizeof ts);
        const int64_t kNanoSecondsPerSecond = 1000000000;
        const int kMinInterval = 100000;
        int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
        if (nanoseconds < kMinInterval)
            nanoseconds = kMinInterval;
        ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
        return ts;
    }
private:
    double interval;
    function<void()> timercallback;
};

void print(const char* msg) {
    static std::map<const char*, Timestamp> lasts;
    Timestamp& last = lasts[msg];
    Timestamp now = Timestamp::now();
    printf("%s tid %d %s delay %f\n", now.toString().c_str(), CurrentThread::gettid(),
            msg, Timestamp::timeDifference(now, last));
    last = now;
}

int main() {
    EventLoop loop;
    PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
    timer.start();
    loop.addTimer(1000, std::bind(print, "EventLoop::runEvery"), 1000);
    loop.loop();
    return 0;
}

#else
// timerfd linux only
int main() { return 0; }

#endif