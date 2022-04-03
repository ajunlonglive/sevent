#include "CurrentThread.h"
using namespace sevent;

thread_local pid_t CurrentThread::tid = 0;