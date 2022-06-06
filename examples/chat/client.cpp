#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoopThread.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#include "LengthHeaderCodec.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class ChatClientHandler : public LengthHeaderCodec {
public:
    void write(const string &msg) {
        LengthHeaderCodec::send(connection, msg);
    }
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        lock_guard<mutex> lg(mtx);
        connection = conn;
    }
    void onMessageStr(const TcpConnection::ptr &conn, const std::string &msg) override {
        printf("<<< %s\n", msg.c_str());
    }
    void onClose(const TcpConnection::ptr &conn) {
        lock_guard<mutex> lg(mtx);
        connection.reset();
    }
private:
    mutex mtx;
    TcpConnection::ptr connection;
};

int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    string ip = "127.0.0.1";
    int port = 12345;
    if (argc > 1)
        ip = argv[1];
    if (argc > 2)
        port = atoi(argv[2]);
    EventLoopThread loopThread;
    EventLoop *loop = loopThread.startLoop("ChatClient");
    InetAddress addr(ip, static_cast<uint16_t>(port));
    TcpClient client(loop, addr);    
    ChatClientHandler handler;
    client.setTcpHandler(&handler); 
    client.connect();

    string line;
    while (std::getline(std::cin, line)) {
      handler.write(line);
    }

    client.shutdown();
    this_thread::sleep_for(chrono::seconds(1)); // 等待关闭
    return 0;
}