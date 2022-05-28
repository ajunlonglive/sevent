#include <iostream>
#include <thread>
#include <chrono>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

const char *buf = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/plain\r\n"
                  "Transfer-Encoding: chunked\r\n"
                  "\r\n"
                  "25  \r\n"
                  "This is the data in the first chunk\r\n"
                  "\r\n"
                  "1C\r\n"
                  "and this is the second one\r\n"
                  "\r\n"
                  "0  \r\n"
                  "\r\n";

class MyHandler : public TcpHandler {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        cout<<"send buf"<<endl;
        conn->send(buf);
        this_thread::sleep_for(3s);
        cout<<"send 404"<<endl;
        conn->send("HTTP/1.1 404 Not Found\r\n\r\n");
        this_thread::sleep_for(3s);
        cout<<"close"<<endl;
        conn->shutdown();
    }
};

int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    const char *ip = "127.0.0.1";
    int port = 12345;
    if (argc > 2) {
        ip = argv[1];
        port = atoi(argv[2]);
    }
    EventLoop loop;
    InetAddress serverAddr(ip, static_cast<uint16_t>(port)); 
    TcpClient client(&loop, serverAddr);
    MyHandler handler;
    client.setTcpHandler(&handler);
    client.connect();
    loop.loop();
    return 0;
}