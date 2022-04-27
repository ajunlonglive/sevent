#ifndef SEVENT_NET_TIMERMANAGER_H
#define SEVENT_NET_TIMERMANAGER_H

#include "../base/noncopyable.h"
#include "Channel.h"
#include "Timer.h"
#include "TimerId.h"
#include <memory>
#include <set>
#include <vector>
namespace sevent {
namespace net {
class EventLoop;
class TimerId;

class TimerManager : noncopyable {
public:
    TimerManager(EventLoop *loop);
    ~TimerManager();

    // 必须线程安全
    TimerId addTimer(std::function<void()> cb, Timestamp expired,
                     double interval);
    // 必须线程安全
    void cancel(TimerId timerId);

private:
    // 获取并处理timeout的Timer
    void handleRead();
    void readTimerfd();
    void resetExpired(Timestamp expired);
    void resetTimerExpired(std::vector<Timer::ptr> &list, Timestamp now);
    bool insert(Timer::ptr &timer);
    std::vector<Timer::ptr> getExpired(Timestamp now);

    int initTimerfd();

    void addTimerInLoop(Timer::ptr &timer);
    void cancelInLoop(const TimerId &timerId);

private:
    EventLoop *ownerLoop;
    const int timerfd;
    Channel timerChannel;
    // Timer::ptr  = shared_ptr<Timer>
    using TimerSet = std::set<std::shared_ptr<Timer>, Timer::Comparator>;
    TimerSet timers;
};

} // namespace net
} // namespace sevent

#endif