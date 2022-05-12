#include "sevent/net/EventLoopThread.h"
#include "sevent/net/EventLoop.h"
using namespace std;
using namespace sevent;
using namespace sevent::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb) 
    :loop(nullptr),isLooping(false), latch(1), callback(cb) {
}
EventLoopThread::~EventLoopThread() {
    if (isLooping)
        loop->quit();
    thd.join();
}
EventLoop *EventLoopThread::startLoop(const string &name) {
    thd = thread(&EventLoopThread::start, this, name);
    latch.wait();
    return loop;
}

void EventLoopThread::start(const string &name) {
    // thd线程
    EventLoop l(name);
    loop = &l;
    if (callback)
        callback(loop);
    isLooping = true;
    latch.countDown();
    l.loop();
    isLooping = false;
    // 函数结束后,loop成为野指针;若正常退出,应该执行析构函数;否则直接报错?
}