#include "TimerManager.h"

#include "../base/Logger.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include <assert.h>
#include <sys/timerfd.h>
#include <unistd.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

int TimerManager::initTimerfd() {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0)
        LOG_SYSFATAL << "timerfd_create() failed";
    return fd;
}
TimerManager::TimerManager(EventLoop *loop)
    : ownerLoop(loop), timerfd(initTimerfd()), timerChannel(timerfd, loop) {
            // TODO 移动到EventLoop?
    timerChannel.setReadCallback(std::bind(&TimerManager::handleRead, this));
    timerChannel.enableReadEvent();
}
TimerManager::~TimerManager() {
    timerChannel.remove();
    sockets::close(timerfd);
}

// 必须线程安全
TimerId TimerManager::addTimer(std::function<void()> cb,
                               Timestamp expired, double interval) {
    Timer::ptr timer(new Timer(std::move(cb), expired, interval));
    TimerId id(timer);
    ownerLoop->runInLoop(std::bind(&TimerManager::addTimerInLoop, this, std::move(timer)));
    return id;
}
void TimerManager::addTimerInLoop(Timer::ptr &timer) {
    ownerLoop->assertInOwnerThread();
    bool isChanged = insert(timer);
    if (isChanged)
        resetExpired(timer->getExpired());
}

// 必须线程安全
void TimerManager::cancel(TimerId timerId) {
    ownerLoop->runInLoop(
        std::bind(&TimerManager::cancelInLoop, this, std::move(timerId)));
}
void TimerManager::cancelInLoop(const TimerId &timerId) {
    ownerLoop->assertInOwnerThread();
    const Timer::ptr &timer = timerId.getPtr().lock();
    if (timer) {
        TimerSet::iterator it = timers.find(timer);
        if (it != timers.end()) {
            timers.erase(it);
            LOG_TRACE << "TimerManager::cancel() erase timer";
        }
    }
}

void TimerManager::handleRead() {
    // TODO 移动到channel?
    ownerLoop->assertInOwnerThread();
    Timestamp now = Timestamp::now();
    readTimerfd();
    vector<Timer::ptr> expiredList = getExpired(now);
    for (Timer::ptr &item : expiredList) {
        item->run();
    }
    resetTimerExpired(expiredList, now);
}
void TimerManager::readTimerfd() {
    uint64_t t;
    ssize_t ret = sockets::read(timerfd, &t, sizeof(t));
    // LOG_TRACE << "TimerManager::readTimerfd()";
    if (ret != sizeof(t))
        LOG_ERROR << "TimerManager::readTimerfd() - read:" << ret
                  << "bytes instead of 8";
}
std::vector<Timer::ptr> TimerManager::getExpired(Timestamp now) {
    Timer::ptr tp(new Timer(nullptr, now));
    vector<Timer::ptr> expiredList;
    //>=
    TimerSet::iterator it = timers.lower_bound(tp);
    assert(it == timers.end() || now < (*it)->getExpired());
    std::copy(timers.begin(), it, back_inserter(expiredList));
    // expiredList.insert(expiredList.begin(), timers.begin(), it);
    timers.erase(timers.begin(), it);
    return expiredList;
}
void TimerManager::resetExpired(Timestamp expired) {
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
    int ret = timerfd_settime(timerfd, 0, &newValue, nullptr);
    if (ret)
        LOG_SYSERR << "timerfd_settime() failed";
}

void TimerManager::resetTimerExpired(vector<Timer::ptr> &list, Timestamp now) {
    for (Timer::ptr &item : list) {
        if (item->isRepeat()) {
            item->resetExpired(now);
            insert(item);
            // insert(it->release());
        }
    }

    if (!timers.empty()) {
        resetExpired((*timers.begin())->getExpired());
    }
}

bool TimerManager::insert(Timer::ptr &timer) {
    bool isChanged = false;
    Timestamp t = timer->getExpired();
    TimerSet::iterator it = timers.begin();
    if (it == timers.end() || t < (*it)->getExpired())
        isChanged = true;
    pair<TimerSet::iterator, bool> res = timers.insert(timer);
    assert(res.second);(void)res;
    return isChanged;
}
