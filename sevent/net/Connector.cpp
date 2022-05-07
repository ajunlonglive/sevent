#include "Connector.h"
#include "../base/Logger.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "TcpConnection.h"
#include <errno.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

int64_t Connector::nextId = 1;
namespace {
const char *stateArr[] = {"disconnected", "connecting", "connected"};
}

Connector::Connector(EventLoop *loop, const InetAddress &sAddr)
    : Channel(-1, loop), isStop(true), retryMs(minRetryDelayMs), retryCount(-1),
      retryCur(-1), timeout(-1), state(disconnected),
      tcpHandler(nullptr), serverAddr(sAddr) {}
Connector::~Connector() {
    LOG_TRACE << "Connector::~Connector()";
    // TODO
    // 在析构时, 才真正释放TcpConnection?
    // 在onConnection时 发生析构
    // 或者移动到TcpClient析构
}

void Connector::connect() {
    ownerLoop->assertInOwnerThread();
    isStop = false;
    retryCur = retryCount;
    doconnect();
    if (timeout > 0)
        ownerLoop->addTimer(timeout, std::bind(&Connector::doTimeout, shared_from_this()));
}
// 创建新的描述符, 进行connect
void Connector::doconnect() {
    ownerLoop->assertInOwnerThread();
    if (!isStop) {
        int sockfd = sockets::createNBlockfd(serverAddr.family());
        Channel::setFdUnSafe(sockfd);
        int ret =
            sockets::connect(fd, serverAddr.getSockAddr(), sockets::addr6len);
        int err = (ret == 0 ? 0 : errno);
        switch (err) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            LOG_TRACE << "Connector::doconnect() - doconnecting()";
            doconnecting();
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            LOG_TRACE << "Connector::doconnect() - retry()";
            retry();
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "Connector::doconnect() - failed, errno = " << err;
            sockets::close(sockfd);
            setState(disconnected);
            break;
        default:
            LOG_SYSERR << "Connector::doconnect() - unexpected failed, errno = " << err;
            sockets::close(sockfd);
            setState(disconnected);
            break;
        }
    } else {
        LOG_TRACE << "Connector::doconnect() - is stop";
    }
}
void Connector::doconnecting() {
    setState(connecting);
    Channel::enableWriteEvent();
}
void Connector::doTimeout() {
    if (!isStop && (state != connected)) {
        isStop = true;
        if (state == connecting) {
            Channel::remove();
            sockets::close(fd);
        }
        setState(disconnected);
        LOG_TRACE << "Connector::doTimeout(), timeout = " << timeout;
    }
}

// 出错时重试
void Connector::retry() {
    sockets::close(fd);
    setState(disconnected);
    if (!isStop && (retryCur < 0 || retryCur > 0)) {
        if (retryCur > 0)
            --retryCur;
        LOG_TRACE << "Connector::retry() - connecting to "
                  << serverAddr.toStringIpPort() << " in " << retryMs
                  << " milliseconds, retry = "
                  << (retryCur >= 0 ? retryCount - retryCur : -1);
        ownerLoop->addTimer(retryMs / 1000, std::bind(&Connector::doconnect, shared_from_this()));
        retryMs = retryMs * 2 < maxRetryDelayMs ? retryMs * 2 : maxRetryDelayMs;
    } else {
        LOG_TRACE << "Connector::retry() - is stop, isStop = " << isStop
                  << ", retry = " << retryCur;
    }
}
// void Connector::restart() {
//     ownerLoop->assertInOwnerThread();
//     stop();
//     retryMs = minRetryDelayMs;
//     isStop = false;
//     setState(disconnected);
//     doconnect();
//     // TODO connected 状态 无法stop
// }

void Connector::stop() {
    isStop = true;
    ownerLoop->runInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));
}
void Connector::stopInLoop() {
    ownerLoop->assertInOwnerThread();
    isStop = true;
    if (state == connecting) {
        setState(disconnected);
        Channel::remove();
        sockets::close(fd);
        LOG_TRACE << "Connector::stopInLoop(), state = connecting, stopped";
    } else if (state == connected) {
        LOG_TRACE << "Connector::stopInLoop(), state = connected, stop in TcpConnection(if exist)";
        if (connection)
            connection->handleClose(); // 移除事件
    } else {
        LOG_TRACE << "Connector::stopInLoop(), state = disconnected, do nothing, stopped";
    }
}

void Connector::forceClose() {
    isStop = true;
    ownerLoop->runInLoop(
        std::bind(&Connector::forceCloseInLoop, shared_from_this()));
}
void Connector::forceCloseInLoop() {
    isStop = true;
    if (state == connecting) {
        setState(disconnected);
        Channel::remove();
        sockets::close(fd);
        LOG_TRACE << "Connector::forceCloseInLoop(), state = connecting";
    } else if (state == connected) {
        if (connection)
            connection->forceClose();
        LOG_TRACE << "Connector::forceCloseInLoop(), state = connected";
    } else {
        LOG_TRACE << "Connector::forceCloseInLoop(), state = disconnected";
    }
}

void Connector::handleWrite() {
    ownerLoop->assertInOwnerThread();
    if (state == connecting) {
        Channel::remove();
        int err = sockets::getSocketError(fd);
        if (err) {
            LOG_WARN << "Connector::handleWrite() - SO_ERROR = " << err << " "
                     << strerror_tl(err);
            retry();
        } else if (sockets::isSelfConnect(fd)) {
            LOG_WARN << "Connector::handleWrite() - self connect";
            retry();
        } else {
            if (!isStop) {
                onConnection();
            } else {
                sockets::close(fd);
                setState(disconnected);
            }
        }
    } else {
        // LOG_TRACE << "Connector::handleWrite() - do nothing, state = " <<
        // stateArr[state];
    }
}

void Connector::handleError() {
    if (state == connecting) {
        Channel::remove();
        int err = sockets::getSocketError(fd);
        LOG_ERROR << "Connector::handleError(),  fd = " << fd
                  << ", state = " << stateArr[state] << ", SO_ERROR = " << err
                  << " " << strerror_tl(err);
        retry();
    } else {
        LOG_TRACE << "Connector::handleError() - do nothing, state = "
                  << stateArr[state];
    }
}

void Connector::onConnection() {
    setState(connected);
    InetAddress localAddr(sockets::getLocalAddr(fd));
    InetAddress peerAddr(sockets::getPeerAddr(fd));
    connection = make_shared<TcpConnection>(ownerLoop, fd, nextId++, localAddr,
                                            peerAddr);
    connection->setTcpHolder(this);
    connection->setTcpHandler(tcpHandler);
    connection->onConnection();
}

// for TcpConnection::handleClose
void Connector::removeConnection(int64_t id) {
    ownerLoop->assertInOwnerThread();
    connection.reset();
    setState(disconnected);
    LOG_TRACE << "Connector::removeConnection(), id = " << id;
}