#include "TimerManager.h"

#include "../base/Logger.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "TimerfdChannel.h"
#include <assert.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

TimerManager::TimerManager(EventLoop *loop)
    : ownerLoop(loop), timerfdChannel(createChannel()) {}
TimerManager::~TimerManager() {}

TimerfdChannel *TimerManager::createChannel() {
    return new TimerfdChannel(ownerLoop,std::bind(&TimerManager::handleExpired, this));
}

TimerId TimerManager::addTimer(std::function<void()> cb, Timestamp expired, int64_t interval) {
    Timer::ptr timer(new Timer(std::move(cb), expired, interval));
    TimerId id(timer);
    ownerLoop->runInLoop(std::bind(&TimerManager::addTimerInLoop, this, std::move(timer)));
    return id;
}

void TimerManager::addTimerInLoop(Timer::ptr &timer) {
    ownerLoop->assertInOwnerThread();
    bool isChanged = insert(timer);
    if (isChanged)
        timerfdChannel->resetExpired(timer->getExpired());
}

void TimerManager::cancel(TimerId timerId) {
    ownerLoop->runInLoop(std::bind(&TimerManager::cancelInLoop, this, std::move(timerId)));
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

void TimerManager::handleExpired() {
    Timestamp now = Timestamp::now();
    vector<Timer::ptr> expiredList = getExpired(now);
    for (Timer::ptr &item : expiredList) {
        item->run();
    }
    resetTimers(expiredList, now);
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

void TimerManager::resetTimers(vector<Timer::ptr> &list, Timestamp now) {
    for (Timer::ptr &item : list) {
        if (item->isRepeat()) {
            item->resetExpired(now);
            insert(item);
        }
    }
    if (!timers.empty()) {
        timerfdChannel->resetExpired((*timers.begin())->getExpired());
    }
}

bool TimerManager::insert(Timer::ptr &timer) {
    bool isChanged = false;
    Timestamp t = timer->getExpired();
    TimerSet::iterator it = timers.begin();
    if (it == timers.end() || t < (*it)->getExpired())
        isChanged = true;
    pair<TimerSet::iterator, bool> res = timers.insert(timer);
    assert(res.second);
    (void)res;
    return isChanged;
}
