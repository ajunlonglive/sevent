#ifndef SEVENT_NET_SSLCLIENTCODEC_H
#define SEVENT_NET_SSLCLIENTCODEC_H

#include "sevent/net/TcpPipeline.h"
namespace sevent {
namespace net {

class SslContext;
class SslHandler;
class SslClientCodec : public PipelineHandler {
public:
    SslClientCodec(SslContext *ctx);
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) override;
    bool onMessage(const TcpConnection::ptr &conn, std::any &msg) override;
    // bool onClose(const TcpConnection::ptr &conn, std::any &msg) override;
    // bool onWriteComplete(const TcpConnection::ptr &conn, std::any &msg) override;
    bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) override;

private:
    SslContext *context;
};

} // namespace net
} // namespace sevent

#endif