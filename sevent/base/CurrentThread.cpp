#include "sevent/base/CurrentThread.h"
#ifndef _WIN32
#include <sys/syscall.h>
#include <unistd.h>
#else
#include <processthreadsapi.h>
#endif
using namespace std;
using namespace sevent;

thread_local int CurrentThread::tid = 0;
thread_local std::string CurrentThread::tidString = "";

int CurrentThread::gettid() {
    if (tid == 0) {
        //缓存tid
        #ifndef _WIN32
        tid = static_cast<int>(::syscall(SYS_gettid));
        #else
        tid = GetCurrentThreadId();
        #endif
        tidString = to_string(tid) + " ";
    }
    return tid;
}
const std::string &CurrentThread::gettidString() {
    if (tidString.empty()) {
        gettid();
    }
    return tidString;   
}