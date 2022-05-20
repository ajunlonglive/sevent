#include <iostream>
#include <thread>
#include <chrono>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class MyHandler : public TcpHandler {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        conn->send("world\n");
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        if (msg == "quit\r\n") {
            conn->send("bye\n");
            conn->shutdown();
        } else if (msg == "shutdown\r\n") {
            conn->getLoop()->quit();
        }else {
            conn->send(msg);        
        }
    }
};

int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 12345); 
    TcpClient client(&loop, serverAddr);
    MyHandler handler;
    client.setTcpHandler(&handler);
    client.connect();
    loop.loop();
    return 0;
}