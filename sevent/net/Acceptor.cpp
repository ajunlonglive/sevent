#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include <errno.h>
#include <string.h>
#include "../base/Logger.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr, ConnectCallBack cb)
    : Channel(sockets::createNBlockfd(addr.family()), loop),
      idleFd(sockets::openIdelFd()), isListening(false), addr(addr),
      connectCallBack(std::move(cb)) {
    sockets::setKeepAlive(fd, true);
    sockets::setReuseAddr(fd, true);
}

void Acceptor::listen() {
    ownerLoop->assertInOwnerThread();
    LOG_TRACE << "Acceptor::listen() - start bind and listen, addr = " << addr.toStringIpPort();
    sockets::bind(fd, addr.getSockAddr(), sockets::addr6len);
    isListening = true;
    sockets::dolisten(fd);
    Channel::enableReadEvent();
}
int Acceptor::accept() {
    InetAddress peerAddr;
    socklen_t addrlen = sockets::addr6len;
    int connfd = sockets::doaccept(fd, peerAddr.getSockAddr(), &addrlen);
    if (connfd >= 0) {
        if (connectCallBack) {
            connectCallBack(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    } else {
        if (errno == EMFILE) {
            sockets::close(idleFd);
            idleFd = sockets::accept(fd, nullptr, nullptr);
            sockets::close(idleFd);
            idleFd = sockets::openIdelFd();
        }
    }
    return connfd;
}
void Acceptor::handleRead() {
    ownerLoop->assertInOwnerThread();
    while (accept() >= 0){
    };
}

Acceptor::~Acceptor() {
    Channel::remove();
    sockets::close(idleFd);
}