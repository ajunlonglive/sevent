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
TcpServer::TcpServer(EventLoop *loop, uint16_t port, int threadNums = 0) 
    : TcpServer(loop, InetAddress(port), threadNums) {}
                    
TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, int threadNums)
    : ownerLoop(loop), tcpHandler(nullptr), nextId(1), started(false),
      acceptor(createAccepptor(loop, listenAddr)),
      workers(new EventLoopWorkers(loop, threadNums)) {}

TcpServer::~TcpServer() {
    ownerLoop->assertInOwnerThread();
    LOG_TRACE << "TcpServer::~TcpServer()";
    using iter = pair<const int64_t, shared_ptr<TcpConnection>>;
    for (iter &item : connections) {
        //TODO
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
    connection->setTcpServer(this);
    connection->setTcpHandler(tcpHandler);
    connections[nextId++] = connection;
    // TODO removeConnection
    worker->runInLoop(std::bind(&TcpConnection::onConnection, connection));
}

vector<EventLoop *> &TcpServer::getWorkerLoops() {
    return workers->getWorkerLoops();
}

void TcpServer::setWorkerInitCallback(
    const std::function<void(EventLoop *)> &cb) {
    initCallBack = cb;
}