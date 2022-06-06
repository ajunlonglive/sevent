#include <iostream>
#include <string>
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

class ChargenHandler : public TcpHandler {
public:
    ChargenHandler(EventLoop *loop) : transferred(0), startTime(Timestamp::now()) {
        string line;
        for (int i = 33; i < 127; ++i)
            line.push_back(char(i));
        line += line;
        for (size_t i = 0; i < 127-33; ++i)
            message += line.substr(i, 72) + '\n';

        loop->addTimer(3000, std::bind(&ChargenHandler::printThroughput, this), 3000);
    }
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        conn->setTcpNoDelay(true);
        conn->send(message);
        // conn->disableRead();
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "discard - " << msg.size() << " bytes recived at "
                 << conn->getPollTime().toString();
    }

    void onWriteComplete(const TcpConnection::ptr &conn) {
        transferred += message.size();
        conn->send(message);
    } 
    void printThroughput() {
        Timestamp endTime = Timestamp::now();
        double time = Timestamp::timeDifference(endTime, startTime);
        printf("%4.3f MiB/s\n", static_cast<double>(transferred)/time/1024/1024);
        transferred = 0;
        startTime = endTime;
    }
private:
    int count = 0;
    int64_t transferred;
    string message;
    Timestamp startTime;
};
// chargen: 不停地发送数据(发送数据不快过客户端接收数据的速度)
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    ChargenHandler handler(&loop);
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();

    return 0;
}