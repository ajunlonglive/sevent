#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/ssl/SslCodec.h"
#include "sevent/net/ssl/SslContext.h"
#include "sevent/net/http/HttpResponse.h"
#include "sevent/net/http/HttpResponseCodec.h"

#include <stdio.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

class HttpsClientHandler : public PipelineHandler {
public:
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) {
        printf("\nconnected and send\n");
        Buffer buf;
        buf.append("GET / HTTP/1.1\r\n\r\n");
        write(conn, &buf);
        return true;
    }

    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) {
        vector<HttpResponse> &responseList = std::any_cast<vector<HttpResponse> &>(msg);
        for (HttpResponse &response : responseList) {
            printf("====== recv-response: ======\n%s", response.toString().c_str());
            printf("\n======    recv-end    ======\n");
        }
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

    // 初始化sslcontext
    SslContext context;

    EventLoop loop;
    InetAddress addr(ip, port);

    TcpPipeline pipeline;
    SslCodec sslCodec(&context);
    HttpResponseCodec httpCodec;
    HttpsClientHandler handler;
    pipeline.addLast(&sslCodec);
    pipeline.addLast(&httpCodec);
    pipeline.addLast(&handler);

    TcpClient client(&loop, addr);
    client.setTcpHandler(&pipeline);

    client.connect();
    loop.loop();
    return 0;
}