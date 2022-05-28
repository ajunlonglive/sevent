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

const char *buf1 = "GET http://a%12:b!&*$@hypnotoad.org:1234/toto?p=1&s=2 HTTP/1.1\r\n"
                   "Connection: Upgrade\r\n"
                   "User-agent: Mozilla/1.1N\r\n"
                   "Content-Length: 15\r\n"
                   "\r\n"
                   "sweet post body";
const char *buf2 = "POST /two_chunks_mult_zero_end HTTP/1.1\r\n"
                   "Transfer-Encoding: chunked\r\n"
                   "\r\n"
                   "5\r\nhello\r\n"
                   "6\r\n world\r\n"
                   "000\r\n"
                   "\r\n";

class MyHandler : public TcpHandler {
public:
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        conn->send(buf1);
        conn->send(buf2);
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