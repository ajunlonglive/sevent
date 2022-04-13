#ifndef SEVENT_BASE_CURRENTTHREAD_H
#define SEVENT_BASE_CURRENTTHREAD_H

#include <stdio.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
namespace sevent {

class CurrentThread {
public:
    static pid_t gettid() {
        if (tid == 0) {
            //缓存tid
            tid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
        return tid;
    }

private:
    thread_local static pid_t tid;
};

} // namespace sevent

#endif