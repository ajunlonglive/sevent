#ifndef SEVENT_NET_CONNECTOR_H
#define SEVENT_NET_CONNECTOR_H

#include "Channel.h"
#include "InetAddress.h"
#include "TcpConnectionHolder.h"
#include <stdint.h>
#include <functional>
#include <memory>

namespace sevent {
namespace net {
class EventLoop;
class TcpConnection;
class TcpHandler;

// Connector的主要被TcpClient持有,TcpClient析构时, 调用stop
class Connector : public Channel,
                  public std::enable_shared_from_this<Connector>,
                  public TcpConnectionHolder {
public:
    
    Connector(EventLoop *loop, const InetAddress &sAddr);
    ~Connector();

    void connect();
    // void restart();
    // 关闭连接, 包括已经连接或正在连接
    void stop();
    void forceClose();
    
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }
    // 正在连接时的超时时间(秒), 默认-1(<=0 无限)
    void setTimeout(double seconds) { timeout = seconds; }
    void retry();
    // 出错时重试次数, 默认-1(无限次), 0(不重试)
    // 出错时会重试: 0.5s, 1s, 2s, .. 30s (比如:Connection refused)
    void setRetryCount(int count) { retryCount = count; }    
    const InetAddress &getServerAddr() const { return serverAddr; }

private:
    enum State { disconnected, connecting, connected };
    void setState(State s) { state = s; }
    void doconnecting();
    void doconnect();
    void doTimeout();
    void onConnection();
    void stopInLoop();
    void forceCloseInLoop();


    void handleWrite() override;
    void handleError() override;
    void removeConnection(int64_t) override;

private:
    bool isStop;
    int retryMs;
    int retryCount;
    int retryCur;
    double timeout;
    State state;
    TcpHandler *tcpHandler;
    InetAddress serverAddr;
    std::shared_ptr<TcpConnection> connection;
    static int64_t nextId;
    static const int maxRetryDelayMs = 30 * 1000; // 30000ms
    static const int minRetryDelayMs = 500; // TODO 500ms
};

} // namespace net
} // namespace sevent

#endif