#include <iostream>
#include <algorithm>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/EventLoopWorkers.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#include "LengthHeaderCodec.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

int g_connections = 0;
atomic<int> g_aliveConnections;
atomic<int> g_messagesReceived;
Timestamp g_startTime;
std::vector<Timestamp> g_receiveTime;
EventLoop* g_loop;
std::function<void()> g_statistic;

class ChatClientHandler : public LengthHeaderCodec {
public:
    Timestamp getReceiveTime() { return receiveTime; }

private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        connection = conn;
        if (++g_aliveConnections == g_connections) {
            LOG_INFO << "all connect";
            conn->getLoop()->addTimer(
                5000, std::bind(&ChatClientHandler::send, this));
        }
    }
    void onMessageStr(const TcpConnection::ptr &conn, const std::string &msg) override {
        receiveTime = conn->getPollTime();
        int received = ++g_messagesReceived;
        if (received == g_connections) {
            Timestamp endTime = Timestamp::now();
            LOG_INFO << "all received " << g_connections << " in "
                    << Timestamp::timeDifference(endTime, g_startTime);
            g_loop->queueInLoop(g_statistic);
        } else if (received % 1000 == 0) {
            LOG_DEBUG << received;
        }
    }
    void onClose(const TcpConnection::ptr &conn) { connection.reset(); }

    void send() {
        g_startTime = Timestamp::now();
        LengthHeaderCodec::send(connection, "hello");
        LOG_INFO << "send";
    }
private:
    Timestamp receiveTime;
    mutex mtx;
    TcpConnection::ptr connection;
};

class ChatClient {
public:
    ChatClient(EventLoop *loop, const InetAddress &addr) : client(loop, addr) {
        client.setTcpHandler(&handler);
    }
    void connect() { client.connect(); }
    void shutdown() { client.shutdown(); }
    Timestamp getReceiveTime() { return handler.getReceiveTime(); }

private:
    ChatClientHandler handler;
    TcpClient client;
};

void statistic(const std::vector<std::unique_ptr<ChatClient>>& clients) {
    double recv = Timestamp::timeDifference(
        clients[clients.size() - 1]->getReceiveTime(), g_startTime);
    printf("total connection = %d, all received in %.6f\n", static_cast<int>(clients.size()), recv);
    std::vector<double> seconds(clients.size());
    for (size_t i = 0; i < clients.size(); ++i)
        seconds[i] = Timestamp::timeDifference(clients[i]->getReceiveTime(), g_startTime);
    
    std::sort(seconds.begin(), seconds.end());

    for (size_t i = 0; i < clients.size(); i += max(static_cast<size_t>(1), clients.size()/20))
        printf("%6zd%% %.6f\n", i*100/clients.size(), seconds[i]);

    if (clients.size() >= 100)
        printf("%6d%% %.6f\n", 99, seconds[clients.size() - clients.size()/100]);
    printf("%6d%% %.6f\n", 100, seconds.back());
}

int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    g_loop = &loop;
    g_connections = 10;
    int threadNum = 3;
    if (argc > 2) {
        threadNum = atoi(argv[1]);
        g_connections = atoi(argv[2]);
    }
    EventLoopWorkers workers(&loop, threadNum);
    workers.start(nullptr);

    g_receiveTime.reserve(g_connections);
    std::vector<std::unique_ptr<ChatClient>> clients(g_connections);
    g_statistic = std::bind(statistic, std::ref(clients));

    InetAddress addr("127.0.0.1", 12345);

    for (int i = 0; i < g_connections; ++i) {
      clients[i].reset(new ChatClient(workers.getNextLoop(), addr));
      clients[i]->connect();
      this_thread::sleep_for(chrono::nanoseconds(200));
    }

    loop.loop();
    return 0;
}