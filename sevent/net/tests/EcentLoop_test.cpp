#include <iostream>
#include <thread>
#include "sevent/base/CurrentThread.h"
#include "sevent/net/EventLoop.h"
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

void callback() {
  printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::gettid());
  EventLoop anotherLoop("callbackLoop");
}
void func() {
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::gettid());
    EventLoop loop("funcLoop");
    loop.addTimer(1000, callback);
    loop.loop();
}

int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::gettid());
    EventLoop loop;
    thread t(func);

    loop.loop();
    t.join();
    return 0;
}