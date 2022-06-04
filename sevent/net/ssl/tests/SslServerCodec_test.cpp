#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/ssl/SslCodec.h"
#include "sevent/net/ssl/SslContext.h"

#include <stdio.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html; charset=UTF-8\r\n"
                       "Content-Length: 13\r\n\r\n"
                       "hello world\r\n";

class MyPipelineHandler : public PipelineHandler {
public:
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) {
        printf("\nrecv connection\n");
        return true;
    }

    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) {
        Buffer *buf = std::any_cast<Buffer *>(msg);
        printf("recv = %d\n", static_cast<int>(buf->readableBytes()));
        printf("%s\n", buf->readAllAsString().c_str());
        buf->retrieveAll();

        Buffer b;
        b.append(response);
        write(conn, &b);
        return true; 
    }
private:

};

int main(int argc, char **argv){
    uint16_t port = 12345;
    string certFile = "./server.pem";
    string keyFile = "./server.pem";
    printf("Usage: %s <port> <certFilePath> <keyFilePath>\n", argv[0]);
    if (argc > 1) {
        port = static_cast<uint16_t>(atoi(argv[1]));
    }
    if (argc > 3) {
        certFile = argv[2];
        keyFile = argv[3];
    }

    SslContext context(certFile,keyFile);

    EventLoop loop;
    TcpPipeline pipeline;

    SslCodec codec(&context);
    MyPipelineHandler handler;
    pipeline.addLast(&codec);
    pipeline.addLast(&handler);

    TcpServer server(&loop, port);
    server.setTcpHandler(&pipeline);

    server.listen();
    loop.loop();
    return 0;
}