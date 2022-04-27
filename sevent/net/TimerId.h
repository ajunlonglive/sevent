#ifndef SEVENT_NET_TIMERID_H
#define SEVENT_NET_TIMERID_H

#include <memory>
namespace sevent {
namespace net {
class Timer;
class TimerId {
public:
    TimerId(const std::shared_ptr<Timer> &t) : timer(t) {}
    // for TimerManager::cancel 内部使用,检测Timer是否存在
    const std::weak_ptr<Timer> &getPtr() const { return timer; }

private:
    const std::weak_ptr<Timer> timer;
};

} // namespace net
} // namespace sevent

#endif