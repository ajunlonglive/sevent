#ifndef SEVENT_NET_TCPCLIENT_H
#define SEVENT_NET_TCPCLIENT_H

#include "../base/noncopyable.h"
#include "TcpConnectionHolder.h"
#include <stdint.h>
#include <atomic>
#include <memory>
#include <mutex>

namespace sevent {
namespace net {

class Connector;
class EventLoop;
class InetAddress;
class TcpHandler;
class TcpConnection;

class TcpClient : noncopyable, public TcpConnectionHolder {
public:
    TcpClient(EventLoop *loop, const InetAddress &addr);
    ~TcpClient();
    
    void connect();
    // 关闭连接, 包括已经连接或正在连接
    void shutdown();
    // 关闭处于正在连接的fd, (对于已经连接的connection无效)
    void stop();

    void setRetry(bool b) { retry = b; }
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }
    EventLoop *getOwnerLoop() const { return ownerLoop; }
private:
    Connector *createConnector(const InetAddress &addr);
    void onConnection(int sockfd);
    void removeConnection(int64_t id) override;

private:
    bool retry;
    std::atomic<bool> started;
    EventLoop *ownerLoop;
    TcpHandler *tcpHandler;
    int64_t nextId;
    std::shared_ptr<Connector> connector;
    std::shared_ptr<TcpConnection> connection;
    std::mutex mtx;
};

} // namespace net
} // namespace sevent

#endif