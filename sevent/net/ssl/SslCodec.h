#ifndef SEVENT_NET_SSLCODEC_H
#define SEVENT_NET_SSLCODEC_H

#include "sevent/net/TcpPipeline.h"
namespace sevent {
namespace net {

class SslContext;
class SslCodec : public PipelineHandler {
public:
    SslCodec(SslContext *ctx);
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) override;
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override;
    // FIXME: 目前msg只能是Buffer*
    bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) override;
protected:
    SslContext *context;
};

} // namespace net
} // namespace sevent

#endif