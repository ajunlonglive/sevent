#include "sevent/net/ssl/SslCodec.h"

#include "sevent/base/Logger.h"
#include "sevent/net/ssl/SslContext.h"
#include <openssl/err.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;
namespace {
Buffer * convertBuf(std::any &msg) {
    Buffer *buf = nullptr;
    try {
        buf = any_cast<Buffer *>(msg);
        if (buf == nullptr) {
            LOG_ERROR << "SslCodec::convertBuf(), Buffer* = nullptr";
            return nullptr;
        }
    } catch (std::bad_any_cast&) {
        LOG_FATAL << "SslCodec::convertBuf() - bad_any_cast, msg should be Buffer*";
    }
    return buf;
}
}

SslCodec::SslCodec(SslContext *ctx) : context(ctx) {}

bool SslCodec::onConnection(const TcpConnection::ptr &conn, std::any &msg) {
    conn->setContext("SslHandler", make_any<SslHandler>(context));
    conn->setContext("ConnectionMsg", std::move(msg)); // FIXME, 会产生副作用(比如any保存内容会析构)
    if (context->isClient()) {
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
    }
    return false;    
}

bool SslCodec::onMessage(const TcpConnection::ptr &conn, std::any &msg) {
    Buffer *buf = convertBuf(msg);
    if (buf == nullptr)
        return false;
    LOG_LF();
    LOG_TRACE << "SslCodec::onMessage(), recv = " << buf->readableBytes();
    SslHandler &sslHandler = any_cast<SslHandler&>(conn->getContext("SslHandler"));
    SSL *ssl = sslHandler.getSSL();
    while (buf->readableBytes() > 0) {
        // encrypted -> decrypted
        SslHandler::Status status = sslHandler.decrypt(*buf);
        if (status == SslHandler::SSL_WANT) {
            // handshake/renegotiation ?
            if (!SSL_is_init_finished(ssl)) {
                LOG_TRACE << "SslCodec::onMessage() - SSL_is_init_finished is not finished";
                Buffer tmpBuf;
                sslHandler.bioRead(tmpBuf);
                if (tmpBuf.readableBytes())
                    conn->send(std::move(tmpBuf));
                return false;
            } else if (sslHandler.getConnStatus() == SslHandler::ConnectStatus::CONNECTING) {
                // 握手完毕, 执行后续pipeLineHandler的onConnection
                LOG_TRACE << "SslCodec::onMessage() - SSL_is_init_finished is finished";
                sslHandler.setConnStatus(SslHandler::ConnectStatus::CONNECTED);
                std::any &data = conn->getContext("ConnectionMsg");
                TcpPipeline::invoke(&PipelineHandler::onConnection, conn, data, this->nextHandler());
                conn->removeContext("ConnectionMsg");
            }
        } else if (status == SslHandler::SSL_FAIL) {
            LOG_ERROR << "SslCodec::onMessage() - decrypted failed";
            conn->shutdown();
            return false;
        } // TODO else if (SSL_CLOSE)
    }
    // 传递msg
    Buffer *decryptBuf = sslHandler.getDecrypt();
    LOG_TRACE << "SslCodec::onMessage(), remain = "
              << buf->readableBytes() << ", decrypt = " << decryptBuf->readableBytes();
    LOG_LF();
    if (decryptBuf->readableBytes() > 0) {
        // decryptBuf->swap(*buf);
        // return true;
        // 执行后续pipeLineHandler的onMessage
        TcpPipeline::invoke(&PipelineHandler::onMessage, conn, decryptBuf, this->nextHandler());
    }
    return false;
}


bool SslCodec::handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t &size) {
    Buffer *buf = convertBuf(msg);
    if (buf == nullptr)
        return false;
    LOG_LF();
    LOG_TRACE << "SslCodec::handleWrite(), ready to write = " << buf->readableBytes();
    SslHandler &sslHandler = any_cast<SslHandler&>(conn->getContext("SslHandler"));
    SSL *ssl = sslHandler.getSSL();
    SslHandler::Status status = sslHandler.encrypt(*buf);
    if (status == SslHandler::SSL_WANT) {
        // TODO 保存未发送数据
        if (!SSL_is_init_finished(ssl)) {
            Buffer tmpBuf;
            sslHandler.bioRead(tmpBuf);
            conn->send(std::move(tmpBuf));
            return false;
        }
    } else if (status == SslHandler::SSL_FAIL) {
        LOG_ERROR << "SslCodec::handleWrite() - decrypted failed";
        conn->shutdown();
        return false;
    }
    // 传递msg
    Buffer *enCryptBuf = sslHandler.getEnCrypt();
    LOG_TRACE << "SslCodec::handleWrite(), remain = " << buf->readableBytes()
              << ", encrypt = " << enCryptBuf->readableBytes();
    LOG_LF();
    enCryptBuf->swap(*buf);
    return true;
}