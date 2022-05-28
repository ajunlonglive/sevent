#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpPipeline.h"
#include "sevent/net/http/HttpResponse.h"
#include "sevent/net/http/HttpResponseCodec.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

class HttpResponseHandler : public PipelineHandler {
public:
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override {
        HttpResponse &response = any_cast<HttpResponse &>(msg);
        printf("====== recv-response ======\n%s", response.toString().c_str());
        printf("====== recv-response ======\n");
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
    HttpResponseCodec codec;
    HttpResponseHandler handler;
    pipeline.addLast(&codec);
    pipeline.addLast(&handler);
    server.setTcpHandler(&pipeline); 

    server.listen();
    loop.loop();

    return 0;
}