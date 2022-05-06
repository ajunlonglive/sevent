#ifndef SEVENT_NET_TCPSERVER_H
#define SEVENT_NET_TCPSERVER_H

#include "../base/noncopyable.h"
#include "TcpConnectionHolder.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
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

class TcpServer : noncopyable, public TcpConnectionHolder {
public:
    TcpServer(EventLoop *loop, uint16_t port, int threadNums = 0);
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, int threadNums = 0);
    ~TcpServer();

    // 创建worker线程/listen/accept,线程安全
    void start();
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }
    // linux对监听socket设置的部分选项, accept返回的socket将自动继承
    int setSockOpt(int level, int optname, const int *optval); 

    // EventLoopWorker线程初始化时,loop()执行前的回调
    void setWorkerInitCallback(const std::function<void(EventLoop *)> &cb);

    // 当threadNum为0时,ownerLoop就是workerLoop
    EventLoop *getOwnerLoop() const { return ownerLoop; }
    std::vector<EventLoop *> &getWorkerLoops();
    const InetAddress &getListenAddr();

private:
    Acceptor *createAccepptor(EventLoop *loop, const InetAddress &listenAddr);
    void onConnection(int sockfd, const InetAddress &addr);
    // for TcpConnection::handleClose
    void removeConnection(int64_t id) override;
    void removeConnectionInLoop(int64_t id);
private:
    EventLoop *ownerLoop;
    TcpHandler *tcpHandler;
    int64_t nextId;
    std::atomic<bool> started;
    std::unique_ptr<Acceptor> acceptor;
    std::unique_ptr<EventLoopWorkers> workers;
    std::function<void(EventLoop *)> initCallBack;
    std::unordered_map<int64_t, std::shared_ptr<TcpConnection>> connections; 
    // TODO 单纯保存数据, 单调递增, map:id+string?
};

} // namespace net
} // namespace sevent

#endif