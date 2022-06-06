#include "pubsub.h"
#include "hubCodec.h"

using namespace std;
using namespace pubsub;
using namespace sevent;
using namespace sevent::net;

PubSubClient::PubSubClient(EventLoop *loop, const InetAddress &addr)
     : client(loop, addr){
    client.setTcpHandler(this);
}
void PubSubClient::connect() { client.connect(); }
void PubSubClient::shutdown() { client.shutdown(); }
bool PubSubClient::connected() { return conn && conn->isConnected(); }

bool PubSubClient::subscribe(const string &topic, const SubscribeCallback& cb) {
    string message = "sub " + topic + "\r\n";
    subscribeCb = cb;
    return send(message);
}
void PubSubClient::unsubscribe(const string &topic) {
    string message = "unsub " + topic + "\r\n";
    send(message);
}
bool PubSubClient::publish(const string &topic, const string &content) {
    string message = "pub " + topic + "\r\n" + content + "\r\n";
    return send(message);
}

void PubSubClient::onConnection(const TcpConnectionPtr &conn) {
    this->conn = conn;
    if (connectionCb)
        connectionCb(this);
}
void PubSubClient::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
    ParseResult result = kSuccess;
    while (result == kSuccess) {
        string cmd;
        string topic;
        string content;
        result = parseMessage(buf, &cmd, &topic, &content);
        if (result == kSuccess) {
            if (cmd == "pub" && subscribeCb) {
                subscribeCb(topic, content, conn->getPollTime());
            }
        } else if (result == kError) {
            conn->shutdown();
        }
    }    
}

void PubSubClient::onClose(const TcpConnectionPtr &conn) { this->conn.reset(); }

bool PubSubClient::send(const string &msg) {
    bool success = false;
    if (connected()) {
        conn->send(msg);
        success = true;
    }
    return success;
}