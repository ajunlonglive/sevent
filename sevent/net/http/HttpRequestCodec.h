#ifndef SEVENT_NET_HTTPREQUESTHANDLER_H
#define SEVENT_NET_HTTPREQUESTHANDLER_H

#include "sevent/net/http/HttpCodec.h"
namespace sevent {
namespace net {
namespace http {

// 接收request, 发送response(server)
// msg = vector<HttpRequest>
class HttpRequestCodec : public HttpCodec {
public:
    HttpRequestCodec();

private:
    void handleMessage(HttpCodec::ParserList &&parserList, std::any &msg) override;
    // bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) override;
};


} // namespace http
} // namespace net
} // namespace sevent

#endif