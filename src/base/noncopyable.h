#ifndef SEVENT_BASE_NONCOPYABLE_H
#define SEVENT_BASE_NONCOPYABLE_H

namespace sevent {

class noncopyable {
private:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

} // namespace sevent

#endif