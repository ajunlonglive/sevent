#ifndef SEVENT_NET_TCPCONNECTION_H
#define SEVENT_NET_TCPCONNECTION_H

#include "Buffer.h"
#include "Channel.h"
#include "InetAddress.h"
#include <stdint.h>
#include <memory>
namespace sevent {
namespace net {

class EventLoop;
class TcpHandler;
class TcpServer;

class TcpConnection : public Channel,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    using ptr = std::shared_ptr<TcpConnection>;
    TcpConnection(EventLoop *loop, int sockfd, int64_t connId,
                  const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    void onConnection();

    int64_t getId() { return id; }
    void setTcpServer(TcpServer *server) { tcpServer = server; }
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }
    const InetAddress &getLocalAddr() const { return localAddr; }
    const InetAddress &getPeerAddr() const { return peerAddr; }

private:
    enum State { disConnected, connecting, connected, disconnecting };
    void setTcpState(State s) { state = s; }
    void handleRead() override;

private:
    int64_t id;
    State state;
    TcpServer *tcpServer;
    TcpHandler *tcpHandler;
    const InetAddress localAddr;
    const InetAddress peerAddr;
    Buffer inputBuf;
    Buffer outputBuf;
};

} // namespace net
} // namespace sevent

#endif