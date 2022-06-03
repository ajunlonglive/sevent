#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/http/HttpRequest.h"
#include "sevent/net/http/HttpRequestCodec.h"
#include "sevent/net/http/HttpResponse.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

class HttpRequestHandler : public PipelineHandler {
public:
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override {
        vector<HttpRequest> &resquestList = any_cast<vector<HttpRequest> &>(msg);
        HttpResponse response(HttpVersion::HTTP_1_1, HttpStatus::HTTP_STATUS_OK);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Server", "Sevent");
        response.setHeader("Connection", "keep-alive");
        response.setBody("hello, world");
        for (HttpRequest &resquest : resquestList) {
            if (resquest.getPath() == "/hello") {
                conn->send(response.buildString());
            }
        }
        return true;
    }

private:
};

int main(int argc, char **argv) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    int port = 12345;
    int threadNum = 0;
    if (argc > 2) {
        port = atoi(argv[1]);
        threadNum = atoi(argv[2]);
        Logger::setLogLevel(Logger::WARN);
    }
    EventLoop loop;
    TcpServer server(&loop, static_cast<uint16_t>(port), threadNum);

    TcpPipeline pipeline;
    HttpRequestCodec codec;
    HttpRequestHandler handler;
    pipeline.addLast(&codec);
    pipeline.addLast(&handler);
    server.setTcpHandler(&pipeline);

    server.listen();
    loop.loop();

    return 0;
}