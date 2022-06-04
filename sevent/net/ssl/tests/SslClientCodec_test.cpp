#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/ssl/SslCodec.h"
#include "sevent/net/ssl/SslContext.h"

#include <stdio.h>
// FILE *g_file = fopen("./a.html", "w");

using namespace std;
using namespace sevent;
using namespace sevent::net;

class MyPipelineHandler : public PipelineHandler {
public:
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) {
        printf("\nconnected\n");
        Buffer buf;
        buf.append("GET / HTTP/1.1\r\n\r\n");
        write(conn, &buf);
        return true;
    }

    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) {
        Buffer *buf = std::any_cast<Buffer *>(msg);
        printf("recv = %d\n", static_cast<int>(buf->readableBytes()));
        printf("%s\n", buf->readAllAsString().c_str());
        // fwrite(buf->peek(), 1, buf->readableBytes(), g_file);
        // fflush(g_file);
        buf->retrieveAll();
        return true; 
    }
private:

};

int main(int argc, char **argv){
    string ip = "127.0.0.1";
    uint16_t port = 12345;
    if (argc > 2) {
        ip = argv[1];
        port = static_cast<uint16_t>(atoi(argv[2]));
    } else {
        printf("Usage: %s <ip> <port>\n", argv[0]);
    }
    printf("connect, ip = %s, port = %d\n", ip.c_str(), port);

    SslContext context;

    EventLoop loop;
    InetAddress addr(ip, port);

    TcpPipeline pipeline;
    SslCodec codec(&context);
    MyPipelineHandler handler;
    pipeline.addLast(&codec);
    pipeline.addLast(&handler);

    TcpClient client(&loop, addr);
    client.setTcpHandler(&pipeline);

    client.connect();
    loop.loop();
    return 0;
}