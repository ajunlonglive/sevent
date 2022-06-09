#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/http/HttpRequest.h"
#include "sevent/net/http/HttpRequestCodec.h"
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
        vector<HttpRequest> &resquestList = std::any_cast<vector<HttpRequest> &>(msg);
        for (HttpRequest &resquest : resquestList) {
            printf("====== recv-resquest: ======\n%s", resquest.toString().c_str());
            printf("\n======    recv-end    ======\n");
        }
        return true;
    }
private:
};

int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);

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