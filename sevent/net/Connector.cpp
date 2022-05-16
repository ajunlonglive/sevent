#include "sevent/net/Connector.h"

#include "sevent/base/CommonUtil.h"
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/SocketsOps.h"
#include "sevent/net/TcpConnection.h"
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
        socket_t sockfd = sockets::createNBlockfd(serverAddr.family());
        Channel::setFdUnSafe(sockfd);
        int ret =
            sockets::connect(fd, serverAddr.getSockAddr(), sockets::addr6len);
        int err = (ret == 0 ? 0 : sockets::getErrno());
        switch (err) {
        case 0:
        #ifndef _WIN32
        // Linux sockfd is nonblocking and cannot be completed immediately, EINPROGRESS
        // Unix failed with EAGAIN instead
        // Windows WSAEWOULDBLOCK
        case EINPROGRESS: 
        case EINTR:
        case EISCONN:
        #else
        case WSAEWOULDBLOCK:
        case WSAEINPROGRESS:
        case WSAEINTR:
        case WSAEISCONN:
        #endif
            LOG_TRACE << "Connector::doconnect() - doconnecting() - "
                      << CommonUtil::strerror_tl(err) << " errno = " << err;
            doconnecting();
            break;
        #ifndef _WIN32
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
        #else        
        case WSAEADDRINUSE:
        case WSAEADDRNOTAVAIL:
        case WSAECONNREFUSED:
        case WSAENETUNREACH:
        #endif
            LOG_TRACE << "Connector::doconnect() - retry() - "
                      << CommonUtil::strerror_tl(err) << " errno = " << err;
            retry();
            break;
        #ifndef _WIN32
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
        #else
        case WSAEACCES:
        case WSAEAFNOSUPPORT:
        case WSAEALREADY:
        case WSAEBADF:
        case WSAEFAULT:
        case WSAENOTSOCK:
        #endif
            LOG_SYSERR << "Connector::doconnect() - failed";
            sockets::close(sockfd);
            setState(disconnected);
            break;
        default:
            LOG_SYSERR << "Connector::doconnect() - unexpected failed";
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
        if (timeoutCallBacK)
            timeoutCallBacK();
        LOG_TRACE << "Connector::doTimeout(), timeout = " << timeout << "ms";
    }
}

void Connector::setTimeout(int64_t millisecond, std::function<void()> cb) {
    timeout = millisecond;
    timeoutCallBacK = std::move(cb);
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
        ownerLoop->addTimer(retryMs, std::bind(&Connector::doconnect, shared_from_this()));
        retryMs = retryMs * 2 < maxRetryDelayMs ? retryMs * 2 : maxRetryDelayMs;
    } else {
        LOG_TRACE << "Connector::retry() - is stop, isStop = " << isStop
                  << ", retry = " << retryCur;
    }
}

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
                     << CommonUtil::strerror_tl(err);
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
                  << " " << CommonUtil::strerror_tl(err);
        retry();
    } else {
        LOG_TRACE << "Connector::handleError() - do nothing, state = "
                  << stateArr[state];
    }
}

void Connector::onConnection() {
    // TODO setSocketOpt
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
    // LOG_TRACE << "Connector::removeConnection(), id = " << id;
}