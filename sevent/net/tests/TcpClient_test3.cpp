#include <iostream>
#include <thread>
#include <chrono>
#include "sevent/net/EventLoop.h"
#include "sevent/net/EventLoopThread.h"
#include "sevent/net/TcpClient.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;


int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoopThread loopThread;
    {
        InetAddress serverAddr("127.0.0.1", 12345); // should succeed
        TcpClient client(loopThread.startLoop("threadEvLoop"), serverAddr);
        client.connect();
        this_thread::sleep_for(chrono::seconds(1)); // wait for connect
        client.shutdown();
    }
    this_thread::sleep_for(chrono::seconds(1));

    cout << "finish TcpClient_test3" << endl;
    return 0;
}