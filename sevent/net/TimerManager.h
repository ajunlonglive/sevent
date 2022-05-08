#ifndef SEVENT_NET_TIMERMANAGER_H
#define SEVENT_NET_TIMERMANAGER_H

#include "Timer.h"
#include "TimerId.h"
#include <memory>
#include <set>
#include <vector>
namespace sevent {
namespace net {
class EventLoop;
class TimerfdChannel;
class TimerId;

class TimerManager {
public:
    TimerManager(EventLoop *loop);
    ~TimerManager();

    // 保证在loop线程执行
    TimerId addTimer(std::function<void()> cb, Timestamp expired, int64_t interval);
    // 保证在loop线程执行
    void cancel(TimerId timerId);

private:
    // timerfdChannel的回调
    void handleExpired();
    void resetTimers(std::vector<Timer::ptr> &list, Timestamp now);
    bool insert(Timer::ptr &timer);
    std::vector<Timer::ptr> getExpired(Timestamp now);

    TimerfdChannel *createChannel();
    void addTimerInLoop(Timer::ptr &timer);
    void cancelInLoop(const TimerId &timerId);

private:
    using TimerSet = std::set<std::shared_ptr<Timer>, Timer::Comparator>;
    EventLoop *ownerLoop;
    std::unique_ptr<TimerfdChannel> timerfdChannel;
    TimerSet timers;
};

} // namespace net
} // namespace sevent

#endif