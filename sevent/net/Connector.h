#ifndef SEVENT_NET_CONNECTOR_H
#define SEVENT_NET_CONNECTOR_H

#include "Channel.h"
#include "InetAddress.h"
#include <functional>
#include <memory>

namespace sevent {
namespace net {
class EventLoop;

class Connector : public Channel, public std::enable_shared_from_this<Connector> {
public:
    enum State { disconnected, connecting, connected };
    using ConnectCallBack = std::function<void(int)>;
    Connector(EventLoop *loop, const InetAddress &sAddr, const ConnectCallBack &cb);
    ~Connector();

    void connect();
    void restart();
    // 关闭处于正在连接的fd(移除事件, 并且close)
    void stop();

    State getState() const { return state; }
    const InetAddress &getServerAddr() const { return serverAddr; }
private:
    void setState(State s) { state = s; }
    void doConnecting();
    void connectInLoop();
    void stopInLoop();
    void restartInLoop();
    // 出错时会重试: 0.5s, 1s, 2s, .. 30s
    void retry();

    void handleWrite() override;
    void handleError() override;

private:
    bool isStop;
    int retryMs;
    State state;
    InetAddress serverAddr;
    const std::function<void(int)> connectCallBack;
    static const int maxRetryDelayMs = 30 * 1000;
    static const int minRetryDelayMs = 5000; //TODO 500
};

} // namespace net
} // namespace sevent

#endif