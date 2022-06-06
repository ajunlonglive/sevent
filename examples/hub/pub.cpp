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
string g_topic;
string g_content;

void connection(PubSubClient *client) {
    if (client->connected()) {
        client->publish(g_topic, g_content);
        client->shutdown();
    } else {
        g_loop->quit();
    }
}

int main(int argc, char **argv) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    if (argc > 2) {
        g_topic = argv[1];
        g_content = argv[2];
    } else {
        printf("Usage: %s <topic> <content>\n", argv[0]);
        exit(0);
    }

    EventLoop loop;
    g_loop = &loop;
    InetAddress addr("127.0.0.1", 12345);
    PubSubClient client(&loop, addr);
    client.setConnectionCb(connection);
    client.connect();
    loop.loop();

    return 0;
}