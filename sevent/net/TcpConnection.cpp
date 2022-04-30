#include "TcpConnection.h"

#include "EventLoop.h"
#include "TcpHandler.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpConnection::TcpConnection(EventLoop *loop, int sockfd, int64_t connId,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : Channel(sockfd, loop), id(connId), state(connecting), localAddr(localAddr), peerAddr(peerAddr) {
    
}

TcpConnection::~TcpConnection() {}

void TcpConnection::onConnection() {
    ownerLoop->assertInOwnerThread();
    //TODO tie states
    setTcpState(connected);
    Channel::enableReadEvent();
    if (tcpHandler)
        tcpHandler->onConnection(shared_from_this());
}

void TcpConnection::handleRead() {
    ownerLoop->assertInOwnerThread();
    ssize_t n = inputBuf.readFd(fd);
    if (n > 0) {
        tcpHandler->onMessage(shared_from_this(), &inputBuf);
    } else if (n == 0) {
        // handleClose();
    } else {
        // handleError();
    }
}