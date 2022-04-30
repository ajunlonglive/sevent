#ifndef SEVENT_NET_EVENTLOOPTHREAD_H
#define SEVENT_NET_EVENTLOOPTHREAD_H

#include "../base/CountDownLatch.h"
#include "../base/noncopyable.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>
namespace sevent {
namespace net {
class EventLoop;
class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback());
    ~EventLoopThread();
    EventLoop *startLoop(const std::string &name);
private:
    void start(const std::string &name);

private:
    EventLoop *loop;
    std::atomic<bool> isLooping;
    std::thread thd;
    CountDownLatch latch;
    ThreadInitCallback callback;
};

} // namespace net
} // namespace sevent

#endif