#include <iostream>
#include <thread>
#include <chrono>
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;


void func(EventLoop* loop) {
    InetAddress serverAddr("127.0.0.1", 12345); // should succeed
    TcpClient client(loop, serverAddr);
    client.connect();
    this_thread::sleep_for(chrono::seconds(1));
}


int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    loop.addTimer(2000, std::bind(&EventLoop::quit, &loop));
    thread t(func, &loop);
    t.detach();
    loop.loop();
    cout << "finish TcpClient_test2" << endl;
    return 0;
}