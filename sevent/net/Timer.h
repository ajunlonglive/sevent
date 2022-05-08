#ifndef SEVENT_NET_TIMER_H
#define SEVENT_NET_TIMER_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include <stdint.h>
#include <functional>
#include <memory>
namespace sevent {
namespace net {

class Timer : noncopyable {
public:
    Timer(std::function<void()> cb, Timestamp expired, int64_t interval = 0);

    // now + interval
    void resetExpired(Timestamp now);
    void run() const { if (timerCallback) timerCallback(); }

    Timestamp getExpired() const { return expired; }
    bool isRepeat() const { return repeat; }

    using ptr = std::shared_ptr<Timer>;
    struct Comparator {
        bool operator()(const ptr &lhs, const ptr &rhs) const;
    };

private:
    const std::function<void()> timerCallback;
    Timestamp expired;
    const int64_t interval; //millisecond
    const bool repeat;

};

} // namespace net
} // namespace sevent

#endif