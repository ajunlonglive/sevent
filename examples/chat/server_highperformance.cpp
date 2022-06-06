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
    void setWorkerLoop(vector<sevent::net::EventLoop *> *workers) {
        workerLoops = workers;
    }
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        connections.insert(conn);
    }
    void onMessageStr(const TcpConnection::ptr &conn, const std::string &msg) override {
        for (EventLoop *loop : *workerLoops) {
            loop->queueInLoop(std::bind(&ChatHandler::distributeMsg, this, msg));
        }
    }
    void onClose(const TcpConnection::ptr &conn) { connections.erase(conn); }

    void distributeMsg(const string &msg) {
        for (const TcpConnection::ptr &item : connections) {
            LengthHeaderCodec::send(item, msg);
        }
    }


private:
    vector<EventLoop *> *workerLoops;
    thread_local static set<TcpConnection::ptr> connections;
};

thread_local set<TcpConnection::ptr> ChatHandler::connections;

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
    server.listen(); // 先逐个创建workerLoop, 并运行
    handler.setWorkerLoop(&(server.getWorkerLoops()));
    loop.loop();

    return 0;
}