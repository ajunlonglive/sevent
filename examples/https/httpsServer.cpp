#include <iostream>
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/ssl/SslCodec.h"
#include "sevent/net/ssl/SslContext.h"
#include "sevent/net/http/HttpRequest.h"
#include "sevent/net/http/HttpRequestCodec.h"

#include <stdio.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html; charset=UTF-8\r\n"
                       "Content-Length: 13\r\n\r\n"
                       "hello world\r\n";

Buffer g_buf;
        
class HttpsServerHandler : public PipelineHandler {
public:
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) {
        printf("\nrecv connection\n");
        return true;
    }

    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) {
        vector<HttpRequest> &resquestList = std::any_cast<vector<HttpRequest> &>(msg);
        for (HttpRequest &resquest : resquestList) {
            printf("====== recv-resquest: ======\n%s", resquest.toString().c_str());
            printf("\n======    recv-end    ======\n");
            write(conn, &g_buf);
        }
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

    // response msg
    g_buf.append(response);

    // 初始化sslcontext
    SslContext context(certFile,keyFile);

    EventLoop loop;
    TcpPipeline pipeline;

    SslCodec sslCodec(&context);
    HttpRequestCodec httpCodec;
    HttpsServerHandler handler;
    pipeline.addLast(&sslCodec);
    pipeline.addLast(&httpCodec);
    pipeline.addLast(&handler);

    TcpServer server(&loop, port);
    server.setTcpHandler(&pipeline);

    server.listen();
    loop.loop();
    return 0;
}