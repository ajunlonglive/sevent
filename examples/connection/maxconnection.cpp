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

class EchoHandler : public TcpHandler {
public:
    explicit EchoHandler(int num) : connectNum(0), maxconnectNum(num) {}
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        ++connectNum;
        // 限制最大连接数
        if (connectNum > maxconnectNum) {
            LOG_INFO << "maxconnectNum = " << maxconnectNum
                     << ", connectNum = " << connectNum;
            conn->shutdown();
            conn->forceClose(3000);
        }
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "echo " << msg.size() << " bytes, "
                 << "data received at " << conn->getPollTime().toString();
        conn->send(msg);
    }
    void onClose(const TcpConnection::ptr &conn) { --connectNum; }

private:
    atomic<int> connectNum;
    const int maxconnectNum;
};
// echo: 回声, 限制最大连接数
int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    int maxconnectNum = 3;
    if (argc > 1)
        maxconnectNum = atoi(argv[1]);
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    EchoHandler handler(maxconnectNum); 
    server.setTcpHandler(&handler); 

    server.listen();
    loop.loop();

    return 0;
}