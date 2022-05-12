#ifndef SEVENT_NET_EVENTLOOP_H
#define SEVENT_NET_EVENTLOOP_H

#include "sevent/base/noncopyable.h"
#include "sevent/base/Timestamp.h"
#include "sevent/net/TimerId.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
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
    EventLoop(const std::string &name = "bossLoop");
    ~EventLoop();

    // 只能被EventLoop的所属线程调用(创建EventLoop的线程)
    void loop();
    void quit();
    // 只能被EventLoop所属的ownerThread调用
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    bool isInOwnerThread() const;
    void assertInOwnerThread() const;
    void assertInOwnerThread(const std::string &msg) const;

    // 若在ownerThread中,则直接执行回调;
    // 否则queueInLoop:加入ownerLoop的任务队列(线程安全,下一次任务循环中执行);
    void runInLoop(std::function<void()> cb);
    // 加入任务队列:若在非ownerThread调用或ownerLoop正在执行任务循环,则还要wakeup该ownerLoop(下一次任务循环)
    void queueInLoop(std::function<void()> cb);

    // Timestamp::now, microseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
    // 在time时间运行, 间隔interval毫秒再次运行(0运行一次)
    TimerId addTimer(Timestamp time, std::function<void()> cb, int64_t interval = 0);
    // 在millisecond后运行, 间隔interval毫秒再次运行(0运行一次)
    TimerId addTimer(int64_t millisecond, std::function<void()> cb, int64_t interval = 0);
    void cancelTimer(TimerId timerId);

    const std::string getLoopName() { return loopName; }
    const Timestamp getPollTime() const;

    static const int pollTimeout = 10000; // 10秒(10000 millsecond)
private:
    void wakeup();
    void doPendingTasks();

private:
    bool isTasking;
    std::atomic<bool> isQuit;
    const int threadId;
    int timeout;
    std::unique_ptr<Poller> poller;
    std::unique_ptr<WakeupChannel> wakeupChannel;
    std::unique_ptr<TimerManager> timerManager;
    thread_local static EventLoop *threadEvLoop;

    const std::string loopName;

    std::mutex mtx;
    std::vector<std::function<void()>> pendingTasks; // 任务队列
};

} // namespace net
} // namespace sevent

#endif