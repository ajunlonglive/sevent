#ifndef SEVENT_NET_ACCEPTOR_H
#define SEVENT_NET_ACCEPTOR_H

#include "sevent/net/Channel.h"
#include "sevent/net/InetAddress.h"
#include "sevent/net/util.h"
#include <functional>
namespace sevent {
namespace net {
class EventLoop;
class InetAddress;
// socket/bind/listen/accept
class Acceptor : public Channel {
public:
    using ConnectCallBack = std::function<void(socket_t, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &addr, ConnectCallBack cb);
    ~Acceptor();

    void listen();
    bool isListen() { return isListening; }
    const InetAddress &getAddr() const { return addr; }

private:
    void handleRead() override;
    socket_t accept();

private:
    socket_t idleFd;
    bool isListening;
    const InetAddress addr;
    const std::function<void(socket_t, const InetAddress &)> connectCallBack;
};

} // namespace net
} // namespace sevent

#endif