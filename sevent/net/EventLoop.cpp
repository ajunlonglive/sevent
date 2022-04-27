#include "EventLoop.h"

#include "../base/CurrentThread.h"
#include "../base/Logger.h"
#include "Channel.h"
#include "EpollPoller.h"
#include "PollPoller.h"
#include "SelectPoller.h"
#include "TimerManager.h"
#include "WakeupChannel.h"
#include <assert.h>
#include <vector>

using namespace std;
using namespace sevent;
using namespace sevent::net;

thread_local EventLoop *EventLoop::threadEvLoop = nullptr;

EventLoop::EventLoop()
    : isTasking(false), isQuit(false), threadId(CurrentThread::gettid()),
      poller(new EpollPoller), timerManager(new TimerManager(this)), 
      wakeupChannel(new WakeupChannel(this)) {
    if (!threadEvLoop)
        threadEvLoop = this;
    else
        LOG_FATAL << "EventLoop already exists in this thread, create failed";
}

// TODO
EventLoop::~EventLoop() { threadEvLoop = nullptr; }

void EventLoop::loop() { 
    assertInOwnerThread(); 
    LOG_TRACE << "EventLoop:" << this << " ,start looping";
    while (!isQuit) {
        poller->poll(pollTimeout);
        vector<Channel *> &activeChannels = poller->getActiveChannels();
        for (Channel *channel : activeChannels) {
            channel->handleEvent();
        }
        doPendingTasks();
    }
    LOG_TRACE << "EventLoop:" << this << " ,stop looping";
}

void EventLoop::quit() { 
    isQuit = true; 
    if (!isInOwnerThread())
        wakeup();
}
void EventLoop::updateChannel(Channel *channel) {
    assertInOwnerThread();
    poller->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel) {
    assert(channel->getOwnerLoop() == this);
    assertInOwnerThread();
    poller->removeChannel(channel);
}

void EventLoop::runInLoop(function<void()> cb) {
    if (isInOwnerThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(function<void()> cb) {
    {
        lock_guard<mutex> lg(mtx);
        pendingTasks.push_back(std::move(cb));
    }
    // ownerLoop执行任务队列之前(isTasking),加入任务队列,无需wakeup;若在执行任务队列中,则需要wakeup
    // isTasking只会被同一线程访问,线程安全
    if (!isInOwnerThread() || isTasking)
        wakeup();
}
void EventLoop::wakeup() { wakeupChannel->wakeup(); }
void EventLoop::doPendingTasks() {
    // 通过swap on write,保护pendingTasks:
    // 1.避免执行任务队列时,调用queueInLoop导致死锁
    // 2.减少临界区长度
    vector<function<void()>> tmpTasks;
    isTasking = true;
    {
        lock_guard<mutex> lg(mtx);
        tmpTasks.swap(pendingTasks);
    }
    for (function<void()> &cb : tmpTasks) {
        cb();
    }
    isTasking = false;
}
bool EventLoop::isInOwnerThread() const {
    return threadId == CurrentThread::gettid();
}
void EventLoop::assertInOwnerThread() {
    if (!isInOwnerThread()) {
        LOG_FATAL << "EventLoop::assertInOwnThread - EventLoop:" << this
                  << " was belonged to threadId: " << threadId
                  << " ,current threadId: " << CurrentThread::gettid();
    }
}
// Timestamp::now, microseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
TimerId EventLoop::addTimer(Timestamp time, function<void()> cb, double interval) {
    
    return timerManager->addTimer(std::move(cb), time, interval);
}
// 在second秒后运行, 间隔interval秒再次运行(0运行一次)
TimerId EventLoop::addTimer(double second, std::function<void()> cb, double interval) {
    Timestamp time(Timestamp::addTime(Timestamp::now(), second));
    return timerManager->addTimer(std::move(cb), time, interval);
}
void EventLoop::cancelTimer(TimerId timerId) {
    timerManager->cancel(std::move(timerId));
}