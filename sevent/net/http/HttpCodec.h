#ifndef SEVENT_NET_HTTPCODEC_H
#define SEVENT_NET_HTTPCODEC_H

#include "sevent/net/TcpPipeline.h"
namespace sevent {
namespace net {
namespace http {

class HttpParser;
class HttpCodec : public PipelineHandler {
public:
    HttpCodec(bool isReq);
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) override;
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override;
    using ParserList = std::vector<std::unique_ptr<HttpParser>>;
    // 根据HttpParser(List), 生成HttpRequest或HttpResponse(List), 并且修改msg
    virtual void handleMessage(ParserList &&parserList, std::any &msg) = 0;

private:
    bool isRequest;
};

} // namespace http
} // namespace net
} // namespace sevent

#endif