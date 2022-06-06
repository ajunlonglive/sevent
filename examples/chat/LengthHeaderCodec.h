#ifndef SEVENT_NET_LENGTHHEADERCODEC_H
#define SEVENT_NET_LENGTHHEADERCODEC_H

#include "sevent/base/Logger.h"
#include "sevent/net/TcpHandler.h"
#include <functional>
#include <string>

class LengthHeaderCodec : public sevent::net::TcpHandler {
public:
    // encode and send
    void send(const sevent::TcpConnectionPtr conn, const std::string &msg) {
        sevent::net::Buffer buf;
        buf.append(msg.data(), msg.size());
        buf.prependInt32(static_cast<int32_t>(msg.size()));
        conn->send(std::move(buf));
    }
private:
    // decode
    void onMessage(const sevent::net::TcpConnection::ptr &conn, sevent::net::Buffer *buf) override {
        while (buf->readableBytes() >= headerLen) {
            int32_t len = buf->peekInt32();
            if (len > 65536 || len < 0) {
                LOG_ERROR << "Invalid length, len = " << len;
                conn->shutdown();
                break;
            } else if (buf->readableBytes() >= len + headerLen) {
                buf->retrieve(headerLen);
                std::string message(buf->peek(), len);
                onMessageStr(conn, message);
                buf->retrieve(len);
            } else {
                break; // not enought data
            }
        }
    }

    virtual void onMessageStr(const sevent::TcpConnectionPtr &, const std::string &) = 0;

private:
    const static size_t headerLen = sizeof(int32_t);
};

#endif