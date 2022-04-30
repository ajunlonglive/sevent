#include "EventLoopWorkers.h"

#include "EventLoop.h"
#include "EventLoopThread.h"
#include <assert.h>
#include <string>
using namespace std;
using namespace sevent;
using namespace sevent::net;

EventLoopWorkers::EventLoopWorkers(EventLoop *baseloop, int threadNums) 
    : baseloop(baseloop), threadNums(threadNums), nextIndex(0) {
        //TODO CPU数
}

EventLoopWorkers::~EventLoopWorkers() {

}

void EventLoopWorkers::start(const ThreadInitCallback &cb) {
    baseloop->assertInOwnerThread();
    for (int i = 0; i < threadNums; ++i) {
        string name = "workerLoop" + to_string(i + 1);
        EventLoopThread *evthread = new EventLoopThread(cb);
        EventLoop *loop = evthread->startLoop(name);
        threads.push_back(unique_ptr<EventLoopThread>(evthread));
        workerloops.push_back(loop);
    }
    if (threadNums == 0) {
        workerloops.push_back(baseloop);
        if (cb)
            cb(baseloop);
    }
}

EventLoop *EventLoopWorkers::getNextLoop() {
    baseloop->assertInOwnerThread();
    assert(!workerloops.empty());
    EventLoop *loop = baseloop;
    loop = workerloops[nextIndex];
    ++nextIndex;
    if (nextIndex >= static_cast<int>(workerloops.size()))
        nextIndex = 0;
    return loop;
}