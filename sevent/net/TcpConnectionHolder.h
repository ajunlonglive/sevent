#ifndef SEVENT_NET_TCPCONNECTIONHOLDER_H
#define SEVENT_NET_TCPCONNECTIONHOLDER_H

#include <stdint.h>
namespace sevent {
namespace net {

class TcpConnectionHolder {
public:
    // for TcpConnection::handleClose
    virtual void removeConnection(int64_t id) = 0;
};

} // namespace net
} // namespace sevent

#endif