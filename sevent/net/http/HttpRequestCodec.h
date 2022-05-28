#ifndef SEVENT_NET_HTTPREQUESTHANDLER_H
#define SEVENT_NET_HTTPREQUESTHANDLER_H

#include "sevent/net/TcpPipeline.h"
namespace sevent {
namespace net {
namespace http {

// 接收request, 发送response(server)
class HttpRequestCodec : public PipelineHandler {
private:
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) override;
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override;
    // bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) override;
};


} // namespace http
} // namespace net
} // namespace sevent

#endif