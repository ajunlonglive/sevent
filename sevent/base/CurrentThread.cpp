#include "CurrentThread.h"
#include <sys/syscall.h>
#include <unistd.h>
using namespace sevent;

thread_local pid_t CurrentThread::tid = 0;

pid_t CurrentThread::gettid() {
    if (tid == 0) {
        //缓存tid
        tid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
    return tid;
}