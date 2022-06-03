#include "sevent/net/ssl/SslClientCodec.h"

#include "sevent/base/Logger.h"
#include "sevent/net/SocketsOps.h"
#include "sevent/net/ssl/SslContext.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;


SslClientCodec::SslClientCodec(SslContext *ctx) : context(ctx) {}

bool SslClientCodec::onConnection(const TcpConnection::ptr &conn, std::any &msg) {
    conn->setContext("SslHandler", make_any<SslHandler>(context->sslContext(), true));
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
            return false;
    }
    return false;
}

bool SslClientCodec::onMessage(const TcpConnection::ptr &conn, std::any &msg) {
    Buffer *buf;
    try {
        buf = any_cast<Buffer *>(msg);
        if (buf == nullptr) {
            LOG_ERROR << "SslClientCodec::onMessage(), Buffer* = nullptr";
            return false;
        }
    } catch (std::bad_any_cast&) {
        LOG_FATAL << "SslClientCodec::onMessage() - bad_any_cast, msg should be Buffer*";
    }
    // 开始
    LOG_TRACE << "SslClientCodec::onMessage(), recv = " << buf->readableBytes();
    SslHandler &sslHandler = any_cast<SslHandler&>(conn->getContext("SslHandler"));
    SSL *ssl = sslHandler.getSSL();
    Buffer tmpBuf(buf->size());
    size_t remain = buf->readableBytes();
    while (remain > 0) {
        int n = sslHandler.bioWrite(*buf);
        if (n <= 0) {
            LOG_ERROR << "SslClientCodec::onMessage() - bioWrite failed, ret = " << n;
            conn->shutdown();
            return false;
        }
        remain -= n;
        buf->retrieve(static_cast<size_t>(n));

        // encrypted -> decrypted
        ERR_clear_error();
        int ret = sslHandler.sslRead(tmpBuf);
        SslHandler::Status status = sslHandler.getSslStatus(ret);

        if (status == SslHandler::SSL_WANT) {
            // handshake/renegotiation ?
            // SSL_get_state() TLS_ST_OK
            if (!SSL_is_init_finished(ssl)) {
                LOG_TRACE << "SslClientCodec::onMessage() - SSL_is_init_finished is not finished";
                Buffer tmpBuf2;
                sslHandler.bioRead(tmpBuf2);
                conn->send(std::move(tmpBuf2));
                return false;
            } else if (sslHandler.getConnStatus() == SslHandler::ConnectStatus::CONNECTING) {
                // 握手完毕, 执行后续pipeLineHandler的onConnection
                LOG_TRACE << "SslClientCodec::onMessage() - SSL_is_init_finished is finished";
                sslHandler.setConnStatus(SslHandler::ConnectStatus::CONNECTED);
                PipelineHandler *handler = this->getPipeLine()->getHandlers()->front();
                std::any &data = conn->getContext("ConnectionMsg");
                TcpPipeline::invoke(&PipelineHandler::onConnection, conn, data, handler);
                conn->removeContext("ConnectionMsg");
            }
        } else if (status == SslHandler::SSL_FAIL) {
            LOG_ERROR << "SslClientCodec::onMessage() - sslRead failed";
            conn->shutdown();
            return false;
        }
    }
    // 传递msg
    LOG_TRACE << "SslClientCodec::onMessage(), remain = " << remain;
    tmpBuf.swap(*buf);
    return true;
}

// bool SslClientCodec::onClose(const TcpConnection::ptr &conn, std::any &msg) {

// }

// bool SslClientCodec::onWriteComplete(const TcpConnection::ptr &conn, std::any &msg) {

// }

bool SslClientCodec::handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) {

    return true;
}