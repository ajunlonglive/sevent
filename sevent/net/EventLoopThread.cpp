#include "EventLoopThread.h"
#include "EventLoop.h"
using namespace std;
using namespace sevent;
using namespace sevent::net;

EventLoopThread::EventLoopThread() :loop(nullptr),isLooping(false), latch(1) {
}
EventLoopThread::~EventLoopThread() {
    if (isLooping)
        loop->quit();
    thd.join();
}
EventLoop *EventLoopThread::startLoop() {
    thd = thread(&EventLoopThread::start, this);
    latch.wait();
    return loop;
}

void EventLoopThread::start() {
    //loop 线程
    EventLoop l;
    loop = &l;
    isLooping = true;
    latch.countDown();
    l.loop();
    isLooping = false;
    // 函数结束后,loop成为野指针;若正常退出,应该执行析构函数;否则直接报错?
}