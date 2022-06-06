#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpHandler.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class PingpongHandler : public TcpHandler {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        conn->setTcpNoDelay(true);
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        conn->send(buf);
    }
};

int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    int threadNum = 0;
    if (argc > 1)
        threadNum = atoi(argv[1]);
    EventLoop loop;
    TcpServer server(&loop, 12345, threadNum);    
    PingpongHandler handler;
    server.setTcpHandler(&handler); 

    server.listen();
    loop.loop();

    return 0;
}