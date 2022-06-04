#ifndef SEVENT_NET_HTTPRESPONSEHANDLER_H
#define SEVENT_NET_HTTPRESPONSEHANDLER_H

#include "sevent/net/http/HttpCodec.h"
namespace sevent {
namespace net {
namespace http {

// 接收response, 发送request(client)
// msg =  vector<HttpResponse>
class HttpResponseCodec : public HttpCodec {
public:
    HttpResponseCodec();

private:
    void handleMessage(HttpCodec::ParserList &&parserList, std::any &msg) override;
    bool onClose(const TcpConnection::ptr &conn, std::any &msg) override;
    // bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) override;
};


} // namespace http
} // namespace net
} // namespace sevent

#endif