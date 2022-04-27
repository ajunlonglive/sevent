#ifndef SEVENT_NET_EVENTLOOP_H
#define SEVENT_NET_EVENTLOOP_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "TimerId.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace sevent {
namespace net {
class Channel;
class Poller;
class TimerManager;
class WakeupChannel;

class EventLoop : noncopyable {
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    bool isInOwnerThread() const;
    void assertInOwnerThread();

    // 若在ownerThread中直接执行回调;否则queueInLoop:加入ownerThread的任务队列(线程安全)
    void runInLoop(std::function<void()> cb);
    // 加入任务队列:若在非ownerThread调用或ownerThread正在执行任务队列,则wakeup该ownerLoop
    void queueInLoop(std::function<void()> cb);

    // Timestamp::now, microseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
    // 在time时间运行, 间隔interval秒再次运行(0运行一次)
    TimerId addTimer(Timestamp time, std::function<void()> cb, double interval = 0.0);
    // 在second秒后运行(0立即运行), 间隔interval秒再次运行(0运行一次) (0.1秒->microsecond)
    TimerId addTimer(double second, std::function<void()> cb, double interval = 0.0);
    void cancelTimer(TimerId timerId);

private:
    void wakeup();
    void doPendingTasks();

private:
    bool isTasking;
    std::atomic<bool> isQuit;
    const pid_t threadId;
    std::unique_ptr<Poller> poller;
    std::unique_ptr<TimerManager> timerManager;
    std::unique_ptr<WakeupChannel> wakeupChannel;
    thread_local static EventLoop *threadEvLoop;

    const int pollTimeout = 10000; // 10秒(10000 millsecond)

    std::mutex mtx;
    std::vector<std::function<void()>> pendingTasks; // 任务队列
};

} // namespace net
} // namespace sevent

#endif