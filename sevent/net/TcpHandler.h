#ifndef SEVENT_NET_TCPHANDLER_H
#define SEVENT_NET_TCPHANDLER_H

#include "TcpConnection.h"
namespace sevent {
namespace net {
class TcpHandler {
public:
    // for override
    virtual void onConnection(const TcpConnection::ptr &conn) {}
    virtual void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {}
    virtual void onWriteComplete() {}
    virtual void onClose(const TcpConnection::ptr &conn) {}
    virtual void onHighWaterMark() {}
};

} // namespace net
} // namespace sevent

#endif