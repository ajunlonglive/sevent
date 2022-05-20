#ifndef SEVENT_NET_EVENTLOOPWORKERS_H
#define SEVENT_NET_EVENTLOOPWORKERS_H

#include "sevent/base/noncopyable.h"
#include <functional>
#include <memory>
#include <vector>
namespace sevent {
namespace net {
class EventLoop;
class EventLoopThread;

class EventLoopWorkers {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopWorkers(EventLoop *baseloop, int threadNums);
    ~EventLoopWorkers();
    // ThreadInitCallback:创建线程的时候,loop之前
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    EventLoop *getNextLoop();
    // 当threadNum=0时,baseLoop就是workerLoop
    std::vector<EventLoop *> &getWorkerLoops() { return workerloops; }
    int getThreadNums() { return threadNums; }

private:
    EventLoop *baseloop;
    int threadNums;
    int nextIndex;
    std::vector<EventLoop *> workerloops;
    std::vector<std::unique_ptr<EventLoopThread>> threads;
};

} // namespace net
} // namespace sevent

#endif