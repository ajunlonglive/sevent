#include "sevent/net/TcpServer.h"

#include "sevent/base/Logger.h"
#include "sevent/net/Acceptor.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/EventLoopWorkers.h"
#include "sevent/net/InetAddress.h"
#include "sevent/net/SocketsOps.h"
#include "sevent/net/TcpConnection.h"

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

void TcpServer::listen() {
    if (!started.exchange(true)) {
        workers->start(initCallBack);
        ownerLoop->runInLoop(std::bind(&Acceptor::listen, std::ref(acceptor)));
    }
}
// Acceptor执行的回调
void TcpServer::onConnection(socket_t sockfd, const InetAddress &peerAddr) {
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
// 对监听socket设置这些socket选项,那么accept返回的连接socket将自动继承这些选项:(Linux高性能服务器编程)
// SO_DEBUG、SO_DONTROUTE、SO_KEEPALIVE、SO_LINGER、
// SO_OOBINLINE、SO_RCVBUF、SO_RCVLOWAT、SO_SNDBUF、
// SO_SNDLOWAT、TCP_MAXSEG和TCP_NODELAY
// 而对客户端而言,这些socket选项则应该在调用connect函数之前设置,因为connect调用成功返回之后,TCP三次握手已完成
int TcpServer::setSockOpt(int level, int optname, const void *optval, socklen_t optlen) {
    return sockets::setsockopt(acceptor->getFd(), level, optname, optval, optlen);
}