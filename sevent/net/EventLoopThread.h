#ifndef SEVENT_NET_EVENTLOOPTHREAD_H
#define SEVENT_NET_EVENTLOOPTHREAD_H

#include "../base/CountDownLatch.h"
#include <atomic>
#include <thread>
namespace sevent {
namespace net {
class EventLoop;
class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();
private:
    void start();

private:
    EventLoop *loop;
    std::atomic<bool> isLooping;
    std::thread thd;
    CountDownLatch latch;
};

} // namespace net
} // namespace sevent

#endif