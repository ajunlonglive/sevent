#ifndef SEVENT_NET_ACCEPTOR_H
#define SEVENT_NET_ACCEPTOR_H

#include "Channel.h"
#include <functional>
namespace sevent {
namespace net {
class EventLoop;
class InetAddress;
// socket/bind/listen/accept
class Acceptor : public Channel {
public:
    using ConnectCallBack = std::function<void(int, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &addr, ConnectCallBack cb, bool reuseAddr = true);
    ~Acceptor();

    void listen();
    bool isListen() { return isListening; }

private:
    void handleRead() override;
    int accept();

private:
    int idleFd;
    bool isListening;
    const std::function<void(int,const InetAddress&)> connectCallBack;
};

} // namespace net
} // namespace sevent

#endif