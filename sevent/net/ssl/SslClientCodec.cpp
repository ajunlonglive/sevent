#include "sevent/net/ssl/SslClientCodec.h"

#include "sevent/base/Logger.h"
#include "sevent/net/ssl/SslContext.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

SslClientCodec::SslClientCodec(SslContext *ctx) : SslCodec(ctx) {}

bool SslClientCodec::onConnection(const TcpConnection::ptr &conn, std::any &msg) {
    conn->setContext("SslHandler", make_any<SslHandler>(context));
    conn->setContext("ConnectionMsg", std::move(msg)); // FIXME, 会产生副作用(比如any保存内容会析构)
    SslHandler &sslHandler = any_cast<SslHandler&>(conn->getContext("SslHandler"));
    // ClientHello
    SslHandler::Status status = sslHandler.ssldoHandshake();
    if (status == SslHandler::SSL_WANT) {
        Buffer tmpBuf;
        sslHandler.bioRead(tmpBuf);
        conn->send(std::move(tmpBuf));
    } else if (status == SslHandler::SSL_FAIL) {
            LOG_ERROR << "SslClientCodec::onConnection() - SSL_FAIL";
            conn->shutdown();
    }
    return false;
}