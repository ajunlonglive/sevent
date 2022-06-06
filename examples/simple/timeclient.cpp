#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EndianOps.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class TimeHandler : public TcpHandler {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        if (buf->readableBytes() >= sizeof(int32_t)) {
            time_t stime = buf->readInt32(); // netToHost
            Timestamp t(stime * Timestamp::microSecondUnit);
            LOG_INFO << "Server time = " << stime << ", " << t.toString();
        } else {
            LOG_INFO << "no enought data " << buf->readableBytes() << " bytes";
        }
    }
};
// timeclient: 接收time服务器发送过来的时间, 并打印
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    InetAddress addr("127.0.0.1", 12345);
    TcpClient client(&loop, addr);    
    TimeHandler handler;
    client.setTcpHandler(&handler); 
    client.connect();
    loop.loop();

    return 0;
}