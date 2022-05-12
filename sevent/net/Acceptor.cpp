#include "sevent/net/Acceptor.h"

#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/InetAddress.h"
#include "sevent/net/SocketsOps.h"
#include <errno.h>
#include <string.h>

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
socket_t Acceptor::accept() {
    InetAddress peerAddr;
    socklen_t addrlen = sockets::addr6len;
    socket_t connfd = sockets::doaccept(fd, peerAddr.getSockAddr(), &addrlen);
    if (connfd >= 0) {
        LOG_TRACE << "Acceptor::accept(), fd = " << connfd;
        if (connectCallBack) {
            connectCallBack(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    } else {
        #ifndef _WIN32 // TODO WSAEMFILE 
        if (sockets::getErrno() == EMFILE) {
            sockets::close(idleFd);
            idleFd = sockets::accept(fd, nullptr, nullptr);
            sockets::close(idleFd);
            idleFd = sockets::openIdelFd();
        }
        #endif
        // LOG_TRACE << "Acceptor::accept() - err, fd = " << connfd;
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
    #ifndef _WIN32
    sockets::close(idleFd);
    #endif
}