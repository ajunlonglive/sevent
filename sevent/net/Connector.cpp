#include "Connector.h"
#include "../base/Logger.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include <errno.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

namespace {
const char *stateArr[] = {"disconnected", "connecting", "connected"};
}

Connector::Connector(EventLoop *loop, const InetAddress &sAddr, const ConnectCallBack &cb)
    : Channel(-1, loop), isStop(true), retryMs(minRetryDelayMs),
      state(disconnected), serverAddr(sAddr), connectCallBack(cb) {}
Connector::~Connector() {}

void Connector::connect() {
    isStop = false;
    ownerLoop->runInLoop(std::bind(&Connector::connectInLoop, shared_from_this()));
}
// 创建新的描述符, 进行connect
void Connector::connectInLoop() {
    ownerLoop->assertInOwnerThread();
    if (!isStop) {
        int sockfd = sockets::createNBlockfd(serverAddr.family());
        Channel::setFdUnSafe(sockfd);
        int ret = sockets::connect(fd, serverAddr.getSockAddr(), sockets::addr6len);
        int err = (ret == 0 ? 0 : errno);
        switch (err) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            doConnecting();
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry();
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "Connector::connect() - failed, errno = " << err;
            // TODO setStatus?
            sockets::close(sockfd);
            break;
        default:
            LOG_SYSERR << "Connector::connect() - unexpected failed, errno = " << err;
            // TODO setStatus?
            sockets::close(sockfd);
            break;
        }
    } else {
        LOG_TRACE << "Connector::connectInLoop() - is stop";
    }
}
void Connector::doConnecting() {
    setState(connecting);
    Channel::enableWriteEvent();
}
// handleWrite/Close/connectInLoop,调用retry的时候,必然已经Channel::remove
// 出错时重试
void Connector::retry() {
    sockets::close(fd);
    setState(disconnected);
    if (!isStop) {
        LOG_TRACE << "Connector::retry() - connecting to "
                  << serverAddr.toStringIpPort() << " in " << retryMs
                  << " milliseconds";
        ownerLoop->addTimer(retryMs / 1000, std::bind(&Connector::connectInLoop,
                                                      shared_from_this()));
        // retryMs = std::min<int>(retryMs * 2, maxRetryDelayMs);
        retryMs = retryMs * 2 < maxRetryDelayMs ? retryMs * 2 : maxRetryDelayMs;
    } else {
        LOG_TRACE << "Connector::retry() - is stop";
    }
}

// TODO retry/restart都会调用connectInLoop
void Connector::restart() {
    // TODO 直接ownerLoop
    isStop = true;
    ownerLoop->queueInLoop(std::bind(&Connector::restartInLoop, shared_from_this()));
}
void Connector::restartInLoop() {
    ownerLoop->assertInOwnerThread();
    stopInLoop();
    retryMs = minRetryDelayMs;
    isStop = false;
    setState(disconnected);
    connectInLoop();
}

void Connector::stop() {
    isStop = true;
    ownerLoop->queueInLoop( std::bind(&Connector::stopInLoop, shared_from_this()));
}
// 移除Channel, 并且close
void Connector::stopInLoop() {
    ownerLoop->assertInOwnerThread();
    isStop = true;
    if (state == connecting) {
        setState(disconnected);
        Channel::remove();
        // TODO 若fd设置为负数时,Channel/handle*不进行任何操作,stop可以做成runInLoop(加快在本线程的执行)
        // 避免close带来的影响
        sockets::close(fd);
    }
}

void Connector::handleWrite() {
    ownerLoop->assertInOwnerThread();
    LOG_TRACE << "Connector::handleWrite(), state = " << stateArr[state];
    if (state == connecting) {
        Channel::remove();
        int err = sockets::getSocketError(fd);
        if (err) {
            LOG_WARN << "Connector::handleWrite() - SO_ERROR = " << err << " "
                     << strerror_tl(err);
            retry();
        } else if (sockets::isSelfConnect(fd)){
            LOG_WARN << "Connector::handleWrite() - self connect";
            retry();
        } else {
            setState(connected);
            if (!isStop)
                connectCallBack(fd);
            else
                sockets::close(fd);
        }
    }
}

void Connector::handleError() {
    LOG_ERROR << "Connector::handleError() state = " << stateArr[state];
    if (state == connecting) {
        Channel::remove();
        int err = sockets::getSocketError(fd);
        LOG_ERROR << "Connector::handleError(),  fd = " << fd
                  << ", SO_ERROR = " << err <<" "<< strerror_tl(err);
        retry();
    }
}