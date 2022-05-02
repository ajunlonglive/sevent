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
// TcpServer和用户会持有TcpConnection
class TcpConnection : public Channel,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    using ptr = std::shared_ptr<TcpConnection>;
    TcpConnection(EventLoop *loop, int sockfd, int64_t connId,
                  const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    // Acceptor::handleRead -> TcpServer::onConnection -> onConnection
    void onConnection();
    void forceClose();
    // for TcpServer
    void removeItself();

    int64_t getId() { return id; }
    void setTcpServer(TcpServer *server) { tcpServer = server; }
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }
    const InetAddress &getLocalAddr() const { return localAddr; }
    const InetAddress &getPeerAddr() const { return peerAddr; }

private:
    // TODO connecting->connected
    enum State { disconnected, connecting, connected, disconnecting };
    void setTcpState(State s) { state = s; }
    void handleRead() override;
    void handleWrite() override;
    void handleClose() override;
    void handleError() override;

    void forceCloseInLoop();

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