#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EndianOps.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
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
        time_t now = ::time(0);
        int32_t benow = sockets::hostToNet32(static_cast<int32_t>(now));
        conn->send(&benow, sizeof(benow));
        conn->shutdown();
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "discard - " << msg.size() << " bytes recived at "
                 << conn->getPollTime().toString();
    }
};
// time: 发送当前时间后(二进制数字, 大端), 立即关闭连接
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    TimeHandler handler;
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();

    return 0;
}