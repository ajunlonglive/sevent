#include "sevent/base/Logger.h"
#include "sevent/net/EndianOps.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/EventLoopWorkers.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#include <iostream>
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

atomic<int> g_connectedNum = 0;
class PingpongClient;
class PingpongClients {
public:
    PingpongClients(EventLoop *loop, const InetAddress &addr, int msgSize,
                    int clientCount, int timeout, int workerCount);
    int getClientCount() { return clientCount; }
    int getTimeout() { return timeout; }
    const string &getMsg() { return msg; }
    vector<unique_ptr<PingpongClient>> &getClients() { return clients; }
    void quit() {
        int count = workers.getThreadNums();
        if (count != 0) {
            for (auto &item : workers.getWorkerLoops()) {
                item->queueInLoop(std::bind(&EventLoop::quit, item));
            }
        }
    }

private:
    void handleTimeout();

private:
    int clientCount;
    int timeout;
    string msg;
    EventLoopWorkers workers;
    vector<unique_ptr<PingpongClient>> clients;
};

class PingpongClient : public TcpHandler {
public:
    PingpongClient(EventLoop *loop, const InetAddress &addr,
                   PingpongClients *cls)
        : client(loop, addr), ppclients(cls) {
        client.setTcpHandler(this);
    }

    void connect() { client.connect(); }
    void shutdown() { client.shutdown(); }

private:
    void onConnection(const TcpConnection::ptr &conn) {
        conn->setTcpNoDelay(true);
        conn->send(ppclients->getMsg());
        if (++g_connectedNum == ppclients->getClientCount())
            LOG_WARN << "all connected";
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        ++messageRead;
        bytesRead += buf->readableBytes();
        byteWritten += buf->readableBytes();
        conn->send(buf);
    }
    void onClose(const TcpConnection::ptr &conn) {
        if (--g_connectedNum == 0) {
            LOG_WARN << "all disconnected";
            int64_t totalBytesRead = 0;
            int64_t totalMessagesRead = 0;
            for (auto &item : ppclients->getClients()) {
                totalBytesRead += item->bytesRead;
                totalMessagesRead += item->messageRead;
            }
            LOG_WARN << totalBytesRead << " total bytes read";
            LOG_WARN << totalMessagesRead << " total messages read";
            LOG_WARN << static_cast<double>(totalBytesRead) /
                            static_cast<double>(totalMessagesRead)
                     << " average message size";
            LOG_WARN << static_cast<double>(totalBytesRead) /
                            (ppclients->getTimeout() * 1024 * 1024)
                     << " MiB/s throughput";
            ppclients->quit();
        }
    }

private:
    int64_t bytesRead = 0;
    int64_t byteWritten = 0;
    int64_t messageRead = 0;
    TcpClient client;
    PingpongClients *ppclients;
};

PingpongClients::PingpongClients(EventLoop *loop, const InetAddress &addr,
                                 int msgSize, int clientCount, int timeout,
                                 int workerCount)
    : clientCount(clientCount), timeout(timeout), workers(loop, workerCount) {
    loop->addTimer(timeout * 1000,
                   std::bind(&PingpongClients::handleTimeout, this));
    workers.start();
    for (int i = 0; i < msgSize; ++i) {
        msg.push_back(static_cast<char>(i % 128));
    }
    for (int i = 0; i < clientCount; ++i) {
        PingpongClient *client =
            new PingpongClient(workers.getNextLoop(), addr, this);
        client->connect();
        clients.emplace_back(client);
    }
}
void PingpongClients::handleTimeout() {
    LOG_WARN << "stop";
    for (auto &item : clients) {
        item->shutdown();
    }
}
// pingpong, timeout秒后关闭连接
int main(int argc, char **argv) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    cout<<"Usage: client <msgSize> <clientCount> <timeout> <workerCount> "<<endl;
    int msgSize = 256;
    int clientCount = 1;
    int timeout = 9;
    int workerCount = 0;
    if (argc > 1)
        msgSize = atoi(argv[1]);
    if (argc > 4) {
        clientCount = atoi(argv[2]);
        timeout = atoi(argv[3]);
        workerCount = atoi(argv[4]);
    }

    EventLoop loop;
    InetAddress addr("127.0.0.1", 12345);
    PingpongClients clients(&loop, addr, msgSize, clientCount, timeout,
                            workerCount);
    loop.loop();

    return 0;
}