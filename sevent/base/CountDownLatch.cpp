#include "CountDownLatch.h"

using namespace sevent;

CountDownLatch::CountDownLatch(int count) : count(count){}
void CountDownLatch::wait(){
    std::unique_lock<std::mutex> uLock(this->mtx);
    while (count > 0) {
        cond.wait(uLock);
    }
    uLock.unlock();
}
void CountDownLatch::countDown(){
    std::lock_guard<std::mutex> lg(this->mtx);
    --count;
    if (count == 0)
        cond.notify_all();
}
int CountDownLatch::getCount(){
    std::lock_guard<std::mutex> lg(this->mtx);
    return count;
}