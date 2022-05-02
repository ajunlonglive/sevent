#include "TcpConnection.h"

#include "../base/Logger.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "TcpHandler.h"
#include "TcpServer.h"
#include <assert.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpConnection::TcpConnection(EventLoop *loop, int sockfd, int64_t connId,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : Channel(sockfd, loop), id(connId), state(connecting), localAddr(localAddr), peerAddr(peerAddr) {
    // TODO keepAlive,noDelay,
}

TcpConnection::~TcpConnection() {
    LOG_TRACE << "TcpConnection::~TcpConnection(), id = " << id
              << ", fd = " << fd << ", " << peerAddr.toStringIpPort();
    // 真正关闭fd
    sockets::close(fd);
    assert(state == disconnected);
}

void TcpConnection::onConnection() {
    ownerLoop->assertInOwnerThread();
    //TODO tie states
    assert(state == connecting);
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
    } else if (n == 0) { //对端关闭
        handleClose();
    } else {
        handleError();
    }
}

void TcpConnection::handleWrite() {
    
}
// TODO
// 1.什么情况会发生handleClose?
    // 1.主动调用(forceClose->runInLoop)(当主动关闭时,outputBuf有数据(若write-1,处理EPIPE),应该发送完才handleClose?)
    // 2.read == 0 (对端关闭写(EPOLLRDHUP)/本端关闭读)
    // 3.EPOLLHUP //对端RST/本端和对端都shutdown(WR)/本端shutdown(RDWR)
    // 当outputbuf还有数据继续发送(对端半关闭),发完后关闭自己写端(不再注册事件?)
// 2.handleClose会发生什么?
    // 移除TcpConnection; 先Poller的监听队列/channelMap,后TcpServer的connections
    // (有可能执行多次handleClose, 但是只移除一次)
// 3.TcpConnection析构时(TcpServer和用户会持有),才真正close(fd)
void TcpConnection::handleClose() {
    ownerLoop->assertInOwnerThread();
    if (state == disconnected) 
        return;
    LOG_TRACE << "TcpConnection::handleClose(), id = " << id << ", fd = " << fd
              << ", " << peerAddr.toStringIpPort();
    // TODO outbuf(情况3,不需要写outbuf剩余数据;1,2 是否要继续写outputbuf?)
    removeItself();
    tcpServer->removeConnection(id);
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(fd);
    LOG_ERROR << "TcpConnection::handleError(), id = " << id << ", fd = " << fd
              << ", SO_ERROR = " << err << " ," << peerAddr.toStringIpPort();
}

void TcpConnection::removeItself() {
    ownerLoop->assertInOwnerThread();
    setTcpState(disconnected);
    if (tcpHandler)
        tcpHandler->onClose(shared_from_this());
    Channel::remove(); 
}

void TcpConnection::forceClose() {
    if (state == connected || state == connecting) {
        ownerLoop->runInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}
void TcpConnection::forceCloseInLoop() {
    ownerLoop->assertInOwnerThread();
    if (state == connected || state == connecting) {
        setTcpState(disconnecting);
        handleClose();
    }
}