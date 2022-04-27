#include "Timer.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;


Timer::Timer(function<void()> cb, Timestamp expired, double interval)
    : timerCallback(std::move(cb)), expired(expired), interval(interval),
      repeat(interval > 0.0) {}

void Timer::resetExpired(Timestamp now) {
    if (repeat) {
        expired = Timestamp::addTime(now, interval);
    } else {
        expired = Timestamp();
    }
}


bool Timer::Comparator::operator()(const ptr &lhs, const ptr &rhs) const {
    if (!lhs && !rhs)
        return false;
    if (!lhs)
        return true;
    if (!rhs)
        return false;
    if (lhs->expired < rhs->expired)
        return true;
    if (rhs->expired < lhs->expired) 
        return false;
    return lhs.get() < rhs.get();
    // return lhs->expired < rhs->expired ||
    //        (!(rhs->expired < lhs->expired) && lhs.get() < rhs.get());
}