#include <iostream>
#include <set>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpHandler.h"
#include "LengthHeaderCodec.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class ChatHandler : public LengthHeaderCodec {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        lock_guard<mutex> lg(mtx);
        connections.insert(conn);
    }
    void onMessageStr(const TcpConnection::ptr &conn, const std::string &msg) override {
        lock_guard<mutex> lg(mtx);
        for (const TcpConnection::ptr &item : connections) {
            LengthHeaderCodec::send(item, msg);
        }
    }
    void onClose(const TcpConnection::ptr &conn) { 
        lock_guard<mutex> lg(mtx);
        connections.erase(conn); 
    }

private:
    mutex mtx;
    set<TcpConnection::ptr> connections;
};

int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    int threadNum = 3;
    if (argc > 1)
        threadNum = atoi(argv[1]);
    EventLoop loop;
    TcpServer server(&loop, 12345, threadNum);    
    ChatHandler handler;
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();

    return 0;
}