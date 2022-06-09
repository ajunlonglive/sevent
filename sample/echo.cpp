#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpHandler.h"
#include "sevent/net/TcpServer.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

class EchoHandler : public TcpHandler {
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "echo " << msg.size() << " bytes";
        conn->send(msg);
    }
};
int main(){
#ifdef _WIN32
    // it is not necessary in this case?
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    EchoHandler handler;
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();
    return 0;
}
