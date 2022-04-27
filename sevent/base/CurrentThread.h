#ifndef SEVENT_BASE_CURRENTTHREAD_H
#define SEVENT_BASE_CURRENTTHREAD_H


#include <thread>
namespace sevent {

class CurrentThread {
public:
    static pid_t gettid();

private:
    thread_local static pid_t tid;
};

} // namespace sevent

#endif