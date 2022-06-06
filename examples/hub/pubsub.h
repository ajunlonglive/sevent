#ifndef SEVENT_EXAMPLES_HUB_PUBSUB_H
#define SEVENT_EXAMPLES_HUB_PUBSUB_H

#include "sevent/base/Timestamp.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"

class PubSubClient : public sevent::net::TcpHandler {
public:
    using ConnectionCallback = std::function<void(PubSubClient *)>;
    using SubscribeCallback =
        std::function<void(const std::string &topic, const std::string &content, sevent::Timestamp)>;
public:
    PubSubClient(sevent::net::EventLoop *loop, const sevent::net::InetAddress &addr);
    void connect();
    void shutdown();
    bool connected();

    bool subscribe(const std::string &topic, const SubscribeCallback& cb);
    void unsubscribe(const std::string &topic);
    bool publish(const std::string &topic, const std::string &content);

    void setConnectionCb(ConnectionCallback cb) { connectionCb = cb; }

private:
    void onConnection(const sevent::TcpConnectionPtr &conn);
    void onMessage(const sevent::TcpConnectionPtr &conn, sevent::net::Buffer *buf);
    void onClose(const sevent::TcpConnectionPtr &conn);
    bool send(const std::string &msg);

private:
    sevent::TcpConnectionPtr conn;
    sevent::net::TcpClient client;
    ConnectionCallback connectionCb;
    SubscribeCallback subscribeCb;
};

#endif