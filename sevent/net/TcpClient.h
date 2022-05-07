#ifndef SEVENT_NET_TCPCLIENT_H
#define SEVENT_NET_TCPCLIENT_H

#include "../base/noncopyable.h"
#include <atomic>
#include <memory>

namespace sevent {
namespace net {

class Connector;
class EventLoop;
class InetAddress;
class TcpHandler;

class TcpClient : noncopyable {
public:
    TcpClient(EventLoop *loop, const InetAddress &addr);
    ~TcpClient();
    // connect, 线程安全
    void connect();
    // 关闭连接, 包括已经连接或正在连接(线程安全)
    void shutdown();
    // 关闭连接, 包括已经连接或正在连接(线程安全)
    void forceClose();

    // 出错时重试次数, 默认-1(无限次), 0(不重试)
    // 出错时会重试: 0.5s, 1s, 2s, .. 30s (比如:Connection refused)
    void setRetryCount(int count);
    // 连接的超时时间(秒), 默认-1(<=0 无限)
    void setTimeout(double seconds);
    void setTcpHandler(TcpHandler *handler);
    EventLoop *getOwnerLoop() const { return ownerLoop; }
    const InetAddress &getServerAddr() const;

private:
    Connector *createConnector(const InetAddress &addr);

private:
    std::atomic<bool> started;
    EventLoop *ownerLoop;
    std::shared_ptr<Connector> connector;
};

} // namespace net
} // namespace sevent

#endif