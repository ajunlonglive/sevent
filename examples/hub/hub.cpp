#include "hubCodec.h"
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpHandler.h"
#include "sevent/net/TcpServer.h"
#include <iostream>
#include <map>
#include <set>
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace pubsub;
using namespace sevent;
using namespace sevent::net;

class Topic {
public:
    Topic(const string &topic) : topic(topic) {}
    void add(const TcpConnection::ptr &conn) {
        subs.insert(conn);
        if (lastPubTime.getMicroSecond() > 0)
            conn->send(makeMsg());
    }
    void remove(const TcpConnection::ptr &conn) { subs.erase(conn); }
    void publish(const string &content, Timestamp time) {
        this->content = content;
        lastPubTime = time;
        string message = makeMsg();
        for (auto &item : subs) {
            item->send(message);
        }
    }

private:
    string makeMsg() { return "pub " + topic + "\r\n" + content + "\r\n"; }

    string topic;
    string content;
    Timestamp lastPubTime;
    set<TcpConnection::ptr> subs;
};

class PubSubHandler : public TcpHandler {
public:
    PubSubHandler(EventLoop *loop) {
        loop->addTimer(1000, std::bind(&PubSubHandler::timePublish, this), 1000);
    }

private:
    void onConnection(const TcpConnection::ptr &conn) {
        conn->setContext("subscription", set<string>());
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        ParseResult result = kSuccess;
        while (result == kSuccess) {
            string cmd;
            string topic;
            string content;
            result = parseMessage(buf, &cmd, &topic, &content);
            if (result == kSuccess) {
                if (cmd == "pub") {
                    doPublish(topic, content, conn->getPollTime());
                } else if (cmd == "sub") {
                    LOG_INFO << "subscribes " << topic;
                    doSubscribe(conn, topic);
                } else if (cmd == "unsub") {
                    doUnsubscribe(conn, topic);
                } else {
                    conn->shutdown();
                    result = kError;
                }
            } else if (result == kError) {
                conn->shutdown();
            }
        }
    }
    void onClose(const TcpConnection::ptr &conn) {
        set<string> &connSub = any_cast<set<string>&>(conn->getContext("subscription"));
        for (set<string>::const_iterator it = connSub.begin();
             it != connSub.end();) {
            // doUnsubscribe will erase *it, so increase before calling.
            doUnsubscribe(conn, *it++);
        }
    }

    void timePublish() {
        Timestamp now = Timestamp::now();
        doPublish("utc_time", now.toString(), now);      
    }
    void doSubscribe(const TcpConnection::ptr &conn, const string &topic) {
        set<string> &connSub = any_cast<set<string>&>(conn->getContext("subscription"));
        connSub.insert(topic);
        getTopic(topic).add(conn);
    }
    void doPublish(const string& topic, const string& content, Timestamp time) {
        getTopic(topic).publish(content, time);
    }
    void doUnsubscribe(const TcpConnection::ptr &conn, const string &topic) {
        LOG_INFO << "unsubscribes " << topic;
        set<string> &connSub = any_cast<set<string>&>(conn->getContext("subscription"));
        getTopic(topic).remove(conn);
        connSub.erase(topic);
    }
    Topic &getTopic(const string &topic) {
        map<string, Topic>::iterator it = topics.find(topic);
        if (it == topics.end())
            it = topics.insert(make_pair(topic, Topic(topic))).first;
        return it->second;
    }

private:
    map<string, Topic> topics;
};

int main(int argc, char **argv) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);
    PubSubHandler handler(&loop);
    server.setTcpHandler(&handler);
    server.listen();
    loop.loop();

    return 0;
}