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

class MyHandler : public TcpHandler {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        conn->send("hello\n");
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer &buf) {
        string msg = buf.readAllAsString();
        if (msg == "exit\r\n") {
            cout<<"exit"<<endl;
            conn->send("bye\n");
            conn->shutdown();
        }
        if (msg == "quit\r\n") {
            cout<<"quit"<<endl;
            conn->getLoop()->quit();
        }
        conn->send(msg);        
    }
};

int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    MyHandler handler; // Handler的生命周期应该跟EventLoop/TcpServer一样长(由用户管理)
    server.setTcpHandler(&handler); 

    server.listen();
    loop.loop();

    return 0;
}