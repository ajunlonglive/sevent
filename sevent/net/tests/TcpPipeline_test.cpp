#include <iostream>
#include <set>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpPipeline.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class MyHandler1 : public PipelineHandler {
public:
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override {
        Buffer *buf = any_cast<Buffer *>(msg);
        size_t size = buf->readableBytes();
        if (size > 5) {
            LOG_INFO << "MyHandler1 recv too much, size = " << size << " bytes";
            onError(conn, msg);
            return false;
        } else {
            string s = buf->readAllAsString();
            LOG_INFO << "MyHandler1 recv size = " << s.size() << " bytes";
            msg = std::move(s);
            return true;
        }
        return false;
    }
    bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t size) { 
        string &s = any_cast<string&>(msg);
        s += "MyHandler1\n";
        return true; 
    }
};

class MyHandler2 : public PipelineHandler {
public:
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) override {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
            << conn->getLocalAddr().toStringIpPort();
        return true;
    }

    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override {
        string &s = any_cast<string &>(msg);
        LOG_INFO << "MyHandler2 recv " << s.size() << " bytes : " << s.substr(0, s.size()-2);
        Buffer buf;
        buf.append(s.substr(0, s.size()-2));
        std::any a = buf;
        write(conn, a);
        return true;
    }
    bool handleError(const TcpConnection::ptr &conn, std::any &msg) override {
        LOG_INFO << "MyHandler2 onError shutdown";
        conn->shutdown();
        return false;
    }
    bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t size) { 
        Buffer &s = any_cast<Buffer&>(msg);
        msg = std::move(s.readAllAsString());
        return true; 
    }
};
// onMessage: Buffer -> string
// write: Buffer <- string
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
    
    TcpPipeline pipeline;
    pipeline.addLast(new MyHandler1);
    pipeline.addLast(new MyHandler2);
    server.setTcpHandler(&pipeline); 
    server.listen();
    loop.loop();

    return 0;
}