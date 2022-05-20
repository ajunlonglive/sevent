#ifndef SEVENT_BASE_CURRENTTHREAD_H
#define SEVENT_BASE_CURRENTTHREAD_H


#include <thread>
#include <string>
namespace sevent {
// FIXME 用namespace?
class CurrentThread {
public:
    static int gettid();
    static const std::string &gettidString();

private:
    thread_local static int tid;
    thread_local static std::string tidString;
};

} // namespace sevent

#endif