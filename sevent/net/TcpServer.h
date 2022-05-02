#ifndef SEVENT_NET_TCPSERVER_H
#define SEVENT_NET_TCPSERVER_H

#include "../base/noncopyable.h"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
namespace sevent {
namespace net {
class Acceptor;
class EventLoop;
class EventLoopWorkers;
class InetAddress;
class TcpConnection;
class TcpHandler;

class TcpServer : noncopyable {
public:
    TcpServer(EventLoop *loop, uint16_t port, int threadNums);
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, int threadNums);
    ~TcpServer();

    // 创建worker线程/listen/accept,线程安全
    void start();
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }

    // EventLoopWorker线程初始化时,loop()执行前的回调
    void setWorkerInitCallback(const std::function<void(EventLoop *)> &cb);

    // 当threadNum为0时,ownerLoop就是workerLoop
    EventLoop *getOwnerLoop() const { return ownerLoop; }
    std::vector<EventLoop *> &getWorkerLoops();
    
    // for TcpConnection::handleClose
    void removeConnection(int64_t id);
private:
    Acceptor *createAccepptor(EventLoop *loop, const InetAddress &listenAddr);
    void onConnection(int sockfd, const InetAddress &addr);
    void removeConnectionInLoop(int64_t id);

private:
    EventLoop *ownerLoop;
    TcpHandler *tcpHandler;
    int64_t nextId;
    std::atomic<bool> started;
    std::unique_ptr<Acceptor> acceptor;
    std::unique_ptr<EventLoopWorkers> workers;
    std::function<void(EventLoop *)> initCallBack;
    std::map<int64_t, std::shared_ptr<TcpConnection>> connections;
};

} // namespace net
} // namespace sevent

#endif