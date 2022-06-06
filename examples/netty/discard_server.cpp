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

class DiscardHandler : public TcpHandler {
public:
    DiscardHandler(EventLoop *loop)
     : transferred(0), receivedMsg(0), 
       oldCounter(0), startTime(Timestamp::now()) {
        loop->addTimer(3000, std::bind(&DiscardHandler::printThroughput, this), 3000);
    }
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_TRACE << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        size_t len = buf->readableBytes();
        transferred += len;
        ++receivedMsg;
        buf->retrieveAll();
    }
    void printThroughput() {
        Timestamp endTime = Timestamp::now();
        int64_t newCounter = transferred;
        int64_t bytes = newCounter - oldCounter;
        int64_t msgs = receivedMsg.exchange(0);
        double time = Timestamp::timeDifference(endTime, startTime);
        printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
            static_cast<double>(bytes)/time/1024/1024,
            static_cast<double>(msgs)/time/1024,
            static_cast<double>(bytes)/static_cast<double>(msgs));

        oldCounter = newCounter;
        startTime = endTime;
    }
private:
    atomic<int64_t> transferred;
    atomic<int64_t> receivedMsg;
    int64_t oldCounter;
    Timestamp startTime;
};
// discard: 丢弃所有数据
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
    DiscardHandler handler(&loop);
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();

    return 0;
}