#ifndef SEVENT_NET_SSLCLIENTCODEC_H
#define SEVENT_NET_SSLCLIENTCODEC_H

#include "sevent/net/ssl/SslCodec.h"
namespace sevent {
namespace net {

class SslClientCodec : public SslCodec {
public:
    SslClientCodec(SslContext *ctx);
    bool onConnection(const TcpConnection::ptr &conn, std::any &msg) override;
};

} // namespace net
} // namespace sevent

#endif