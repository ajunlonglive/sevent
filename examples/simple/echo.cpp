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
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "echo " << msg.size() << " bytes, "
                 << "data received at " << conn->getPollTime().toString();
        conn->send(msg);
    }
};
// echo: 回声
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    EchoHandler handler; // Handler的生命周期应该跟EventLoop/TcpServer一样长(由用户管理)
    server.setTcpHandler(&handler); 

    server.listen();
    loop.loop();

    return 0;
}