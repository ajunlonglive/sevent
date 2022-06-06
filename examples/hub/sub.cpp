#include "pubsub.h"
#include "sevent/net/EventLoop.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

EventLoop *g_loop = NULL;
std::vector<string> g_topics;

void subscription(const string &topic, const string &content, Timestamp) {
    printf("%s: %s\n", topic.c_str(), content.c_str());
}

void connection(PubSubClient *client) {
    if (client->connected()) {
        for (std::vector<string>::iterator it = g_topics.begin();
             it != g_topics.end(); ++it) {
            client->subscribe(*it, subscription);
        }
    } else {
        g_loop->quit();
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    g_topics.push_back("mytopic");
    g_topics.push_back("court");

    EventLoop loop;
    g_loop = &loop;
    InetAddress addr("127.0.0.1", 12345);

    PubSubClient client(&loop, addr);
    client.setConnectionCb(connection);
    client.connect();
    loop.loop();

    return 0;
}