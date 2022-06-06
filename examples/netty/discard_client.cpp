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

EventLoop *g_loop;

class DiscardClientHandler : public TcpHandler {
public:
    explicit DiscardClientHandler(int size) : msg(size, 'H') {}
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_TRACE << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        conn->setTcpNoDelay(true);
        conn->send(msg);
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        buf->retrieveAll();
    }
    void onWriteComplete(const TcpConnection::ptr &conn) {
        // LOG_INFO << "write complete " << msg.size();
        conn->send(msg);
    } 
    void onClose(const TcpConnection::ptr &conn) { g_loop->quit(); }
private:
    string msg;
};
// timeclient: 接收time服务器发送过来的时间, 并打印
int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    int msgSize = 256;
    if (argc > 1)
        msgSize = atoi(argv[1]);
    EventLoop loop;
    g_loop = &loop;
    InetAddress addr("127.0.0.1", 12345);
    TcpClient client(&loop, addr);    
    DiscardClientHandler handler(msgSize);
    client.setTcpHandler(&handler); 
    client.connect();
    loop.loop();

    return 0;
}