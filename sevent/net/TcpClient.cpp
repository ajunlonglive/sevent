#include "TcpClient.h"

#include "../base/Logger.h"
#include "Connector.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "TcpConnection.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using std::placeholders::_1;

TcpClient::TcpClient(EventLoop *loop, const InetAddress &addr)
    : retry(false), started(false), ownerLoop(loop), 
      tcpHandler(nullptr), nextId(1), connector(createConnector(addr)) {

}
TcpClient::~TcpClient() {
    LOG_TRACE << "TcpClient::~TcpClient()";
    if (connector->getState() == Connector::connected) {
        bool unique = false;
        shared_ptr<TcpConnection> conn;
        {
            lock_guard<mutex> lg(mtx);
            unique = connection.unique();
            conn = connection;
        }
        if (conn)
            conn->getLoop()->runInLoop(std::bind(&TcpConnection::removeItself, conn));
        if (unique)
            conn->forceClose();
    } else {
        connector->stop();
    }
}
Connector *TcpClient::createConnector(const InetAddress &addr) {
    return new Connector(ownerLoop, addr, std::bind(&TcpClient::onConnection, this, _1));
}

void TcpClient::connect() {
    if (!started.exchange(true)) {
        LOG_TRACE << "TcpClient::connect() - connecting to "
                << connector->getServerAddr().toStringIpPort();
        connector->connect();
    }
}
// 关闭连接, 包括已经连接或正在连接
void TcpClient::shutdown() {
    started = false;
    if (connector->getState() == Connector::connected) {
        {
            lock_guard<mutex> lg(mtx);
            if (connection)
                connection->shutdown();
        }
    } else {
        connector->stop();
    }
}
// 关闭处于正在连接的fd, (对于已经连接的connection无效)
void TcpClient::stop() {
    if (connector->getState() == Connector::connecting)
        connector->stop();
}


void TcpClient::onConnection(int sockfd) {
    ownerLoop->assertInOwnerThread();
    if (started) {
        InetAddress localAddr(sockets::getLocalAddr(sockfd));
        InetAddress peerAddr(sockets::getPeerAddr(sockfd));

        shared_ptr<TcpConnection> conn = make_shared<TcpConnection>(
            ownerLoop, sockfd, nextId, localAddr, peerAddr);
        conn->setTcpHolder(this);
        conn->setTcpHandler(tcpHandler);
        nextId++;
        {
            lock_guard<mutex> lg(mtx);
            this->connection = conn;
        }
        conn->onConnection();
    }
}

void TcpClient::removeConnection(int64_t id) {
    ownerLoop->assertInOwnerThread();
    {
        lock_guard<mutex> lg(mtx);
        connection.reset(); //加锁, 防止其他线程持有空的connection调用方法
    }
    if (retry && started) {
        LOG_TRACE << "TcpClient::removeConnection() - reconnecting to "
                  << connector->getServerAddr().toStringIpPort();
        connector->restart();
    }
}