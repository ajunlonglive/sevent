#include <iostream>
#include <thread>
#include <chrono>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpClient* g_client;

void timeout(){
  LOG_INFO << "timeout";
  g_client->shutdown();
}


int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 2); // no such server
    TcpClient client(&loop, serverAddr);
    g_client = &client;

    loop.addTimer(0, timeout);
    loop.addTimer(1000, std::bind(&EventLoop::quit, &loop));
    client.connect();
    this_thread::sleep_for(chrono::nanoseconds(100 * 1000));
    loop.loop();
    cout << "finish TcpClient_test1" << endl;
    return 0;
}