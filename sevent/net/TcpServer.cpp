#include "TcpServer.h"

#include "../base/Logger.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopWorkers.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "TcpConnection.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
TcpServer::TcpServer(EventLoop *loop, uint16_t port, int threadNums) 
    : TcpServer(loop, InetAddress(port), threadNums) {}

// TODO 多个server 多个server同一个workers?                    
TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, int threadNums)
    : ownerLoop(loop), tcpHandler(nullptr), nextId(1), started(false),
      acceptor(createAccepptor(loop, listenAddr)),
      workers(new EventLoopWorkers(loop, threadNums)) {}

TcpServer::~TcpServer() {
    ownerLoop->assertInOwnerThread();
    LOG_TRACE << "TcpServer::~TcpServer()";
    using iter = pair<const int64_t, shared_ptr<TcpConnection>>;
    for (iter &item : connections) {
        TcpConnection::ptr &conn = item.second;
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::removeItself, conn));
    }
}

Acceptor *TcpServer::createAccepptor(EventLoop *loop, const InetAddress &listenAddr) {
    return new Acceptor(loop, listenAddr, std::bind(&TcpServer::onConnection, this, _1, _2));
}

void TcpServer::start() {
    if (!started.exchange(true)) {
        workers->start(initCallBack);
        ownerLoop->runInLoop(std::bind(&Acceptor::listen, std::ref(acceptor)));
    }
}
// Acceptor执行的回调
void TcpServer::onConnection(int sockfd, const InetAddress &peerAddr) {
    ownerLoop->assertInOwnerThread();
    EventLoop *worker = workers->getNextLoop();

    InetAddress localAddr;
    socklen_t addrlen = sockets::addr6len;
    sockets::getsockname(sockfd, localAddr.getSockAddr(), &addrlen);

    shared_ptr<TcpConnection> connection = make_shared<TcpConnection>(
        worker, sockfd, nextId, localAddr, peerAddr);
    connection->setTcpHolder(this);
    connection->setTcpHandler(tcpHandler);
    connections[nextId++] = connection;
    worker->runInLoop(std::bind(&TcpConnection::onConnection, connection));
}

void TcpServer::removeConnection(int64_t id) {
    ownerLoop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, id));
}
void TcpServer::removeConnectionInLoop(int64_t id) {
    ownerLoop->assertInOwnerThread();
    LOG_TRACE << "TcpServer::removeConnectionInLoop(), id = " << id;
    connections.erase(id);
}

vector<EventLoop *> &TcpServer::getWorkerLoops() {
    return workers->getWorkerLoops();
}

const InetAddress &TcpServer::getListenAddr() { return acceptor->getAddr(); }

void TcpServer::setWorkerInitCallback( const std::function<void(EventLoop *)> &cb) {
    initCallBack = cb;
}

int TcpServer::setSockOpt(int level, int optname, const int *optval) {
    return sockets::setsockopt(acceptor->getFd(), level, optname, optval,
                        static_cast<socklen_t>(sizeof(int)));
}