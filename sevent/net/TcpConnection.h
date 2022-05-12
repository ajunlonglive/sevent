#ifndef SEVENT_NET_TCPCONNECTION_H
#define SEVENT_NET_TCPCONNECTION_H

#include "sevent/net/Buffer.h"
#include "sevent/net/Channel.h"
#include "sevent/net/InetAddress.h"
#include <stdint.h>
#include <memory>
#include <string>
namespace sevent {
namespace net {

class EventLoop;
class TcpHandler;
class TcpClient;
class Connector;
class TcpServer;
class TcpConnectionHolder;

// TcpConnectionPtr会被哪些持有?
// 1.TcpServer::connections
// 2.TcpConnection部分函数执行queueInLoop时,EventLoop任务队列会持有(shared_from_this())
// 3.TcpHandler执行函数时(shared_from_this())
// 4.用户自己持有
class TcpConnection : private Channel,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    using ptr = std::shared_ptr<TcpConnection>;
    TcpConnection(EventLoop *loop, socket_t sockfd, int64_t connId,
                  const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();
    // thread safe
    void send(const void *data, size_t len);
    void send(const std::string &data); 
    void send(const std::string &&data); // std::move(data)
    void send(Buffer &buf); 
    void send(Buffer &&buf); // Buffer::swap
    void shutdown(); // 设置状态disconnecting, 若outputBuf存在数据, 则发送完毕后, 才shutdown(WR)
    void forceClose(); // 调用close, 有可能会丢失数据

    void setHighWaterMark(size_t hwMark) { hightWaterMark = hwMark; }
    int setSockOpt(int level, int optname, const int *optval);
    int64_t getId() { return id; }
    EventLoop *getLoop() { return getOwnerLoop(); }
    const InetAddress &getLocalAddr() const { return localAddr; }
    const InetAddress &getPeerAddr() const { return peerAddr; }

private:
    enum State { connecting, connected, disconnecting, disconnected };
    void setTcpState(State s) { state = s; }
    void handleRead() override;
    void handleWrite() override;
    void handleClose() override;
    void handleError() override;

    void sendInLoop(const void *data, size_t len);
    void sendInLoopStr(const std::string &data);
    void sendInLoopBuf(Buffer &buf);
    void shutdownInLoop();
    void forceCloseInLoop();
    void tie() {}
    // for TcpServer
    friend class TcpServer;
    friend class TcpClient;
    friend class Connector;
    // for Acceptor::handleRead -> TcpServer::onConnection -> onConnection
    void onConnection();
    void setTcpHolder(TcpConnectionHolder *holder) { tcpHolder = holder; }
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }
    void removeItself();

private:
    int64_t id;
    State state;
    TcpConnectionHolder *tcpHolder;
    TcpHandler *tcpHandler;
    const InetAddress localAddr;
    const InetAddress peerAddr;
    Buffer inputBuf;
    Buffer outputBuf;
    size_t hightWaterMark;
};

} // namespace net
} // namespace sevent

#endif