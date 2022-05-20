#include "sevent/net/TcpConnection.h"

#include "sevent/base/CommonUtil.h"
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/SocketsOps.h"
#include "sevent/net/TcpHandler.h"
#include "sevent/net/TcpServer.h"
#include <assert.h>
#include <functional>

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpConnection::TcpConnection(EventLoop *loop, socket_t sockfd, int64_t connId,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : Channel(sockfd, loop),isRead(true), id(connId), state(connecting),
      localAddr(localAddr), peerAddr(peerAddr), hightWaterMark(64 * 1024 * 1024) {
    
}

TcpConnection::~TcpConnection() {
    LOG_TRACE << "TcpConnection::~TcpConnection(), id = " << id
              << ", fd = " << fd << ", " << peerAddr.toStringIpPort();
    // 真正关闭fd // forceClose也会关闭fd, fd = -fd - 1
    if (fd >= 0)
        sockets::close(fd);
    assert(state == disconnected);
}
void TcpConnection::send(const void *data, size_t len) {
    if (state == connected) {
        if (ownerLoop->isInOwnerThread()) {
            sendInLoop(data, len);
        } else {
            string d(static_cast<const char*>(data), len);
            ownerLoop->queueInLoop(std::bind(&TcpConnection::sendInLoopStr,
                                             shared_from_this(), std::move(d)));
        }
    }
}

void TcpConnection::send(const std::string &data) {
    send(data.data(), data.size());
}

void TcpConnection::send(const std::string &&data) {
    if (state == connected) {
        if (ownerLoop->isInOwnerThread()) {
            sendInLoopStr(std::move(data));
        } else {
            ownerLoop->queueInLoop(std::bind(&TcpConnection::sendInLoopStr,
                                             shared_from_this(),
                                             std::move(data)));
        }
    }
}
void TcpConnection::send(Buffer *buf) { send(*buf); }
void TcpConnection::send(Buffer &buf) {
    if (state == connected) {
        if (ownerLoop->isInOwnerThread()) {
            sendInLoopBuf(buf);
        } else {
            ownerLoop->queueInLoop(std::bind(&TcpConnection::sendInLoopStr,
                                             shared_from_this(),
                                             std::move(buf.readAllAsString())));
        }
    }
}

void TcpConnection::send(Buffer &&buf) {
    if (state == connected) {
        if (ownerLoop->isInOwnerThread()) {
            sendInLoopBuf(buf);
        } else {
            ownerLoop->queueInLoop(std::bind(&TcpConnection::sendInLoopBuf,
                                             shared_from_this(),
                                             std::move(buf)));
        }
    }    
}

void TcpConnection::sendInLoopBuf(Buffer &buf) {
    ownerLoop->assertInOwnerThread();
    if (state == disconnected) {
        LOG_TRACE << "disconnected, give up write";
        return;
    }
    size_t len = buf.readableBytes();
    ssize_t n = 0;
    ssize_t remain = len;
    bool isErr = false;
    if (!Channel::isEnableWrite() && outputBuf.readableBytes() == 0) {
        n = sockets::write(fd, buf.peek(), len);
        if (n >= 0) {
            buf.retrieve(n);
            remain -= n;
            if (remain == 0 && tcpHandler != nullptr)
                ownerLoop->queueInLoop(std::bind(&TcpHandler::onWriteComplete,
                                                 tcpHandler,
                                                 shared_from_this()));
        } else {
            n = 0;
            #ifndef _WIN32
            if (sockets::getErrno() != EAGAIN) {
                LOG_SYSERR << "TcpConnection::sendInLoop() - failed";
                if (sockets::getErrno() == EPIPE || sockets::getErrno() == ECONNRESET)
                    isErr = true;
            }
            #else
            if (sockets::getErrno() != WSAEWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop() - failed";
                if (sockets::getErrno() == WSAECONNRESET)
                    isErr = true;
            }
            #endif
        }
    }

    if (!isErr && remain > 0) {
        size_t oldLen = outputBuf.readableBytes();
        if (oldLen + remain >= hightWaterMark && oldLen < hightWaterMark && tcpHandler != nullptr)
            ownerLoop->queueInLoop(std::bind(&TcpHandler::onHighWaterMark,
                                             tcpHandler, shared_from_this(),
                                             oldLen + remain));
        if (oldLen == 0) {
            outputBuf.swap(buf);
        } else  {
            outputBuf.append(buf.peek(), remain);
            buf.retrieveAll();
        }
        if (!Channel::isEnableWrite())
            Channel::enableWriteEvent();
        LOG_TRACE << "TcpConnection::sendInLoopBuf(), fd = "<< fd << " len = " << len << ", write = " 
                  << n << ", remain = " << remain << ", buf = " << (oldLen + remain);
    }
}

void TcpConnection::sendInLoopStr(const string &data) {
    sendInLoop(data.data(), data.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
    ownerLoop->assertInOwnerThread();
    if (state == disconnected) {
        LOG_TRACE << "disconnected, give up write";
        return;
    }
    ssize_t n = 0;
    ssize_t remain = len;
    bool isErr = false;
    // 若不存在写事件, 并且outputBuf不存在数据,直接write;
    // 若write有剩余或已存在写事件,则保存到Buffer,注册写事件(写完后取消写事件)
    // queueInLoop(onWriteComplete), 防止递归调用send
    if (!Channel::isEnableWrite() && outputBuf.readableBytes() == 0) {
        n = sockets::write(fd, data, len);
        if (n >= 0) {
            remain -= n;
            if (remain == 0 && tcpHandler != nullptr)
                ownerLoop->queueInLoop(std::bind(&TcpHandler::onWriteComplete,
                                                 tcpHandler,
                                                 shared_from_this()));
        } else {
            n = 0;
            #ifndef _WIN32
            if (sockets::getErrno() != EAGAIN) {
                LOG_SYSERR << "TcpConnection::sendInLoop() - failed";
                if (sockets::getErrno() == EPIPE || sockets::getErrno() == ECONNRESET)
                    isErr = true;
            }
            #else
            if (sockets::getErrno() != WSAEWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop() - failed";
                if (sockets::getErrno() == WSAECONNRESET)
                    isErr = true;
            }
            #endif
        }
    }

    if (!isErr && remain > 0) {
        size_t oldLen = outputBuf.readableBytes();
        if (oldLen + remain >= hightWaterMark && oldLen < hightWaterMark && tcpHandler != nullptr)
            ownerLoop->queueInLoop(std::bind(&TcpHandler::onHighWaterMark,
                                             tcpHandler, shared_from_this(),
                                             oldLen + remain));
        outputBuf.append(static_cast<const char *>(data) + n, remain);
        if (!Channel::isEnableWrite())
            Channel::enableWriteEvent();
        LOG_TRACE << "TcpConnection::sendInLoop(), fd = "<< fd <<" len = " << len << ", write = " 
                  << n << ", remain = " << remain << ", buf = " << (oldLen + remain);
    }
}

void TcpConnection::shutdown() {
    if (state == connected)
        ownerLoop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
}

void TcpConnection::shutdownInLoop() {
    ownerLoop->assertInOwnerThread();
    if (state == connected) {
        setTcpState(disconnecting);
        LOG_TRACE << "TcpConnection::shutdownInLoop() - set disconnecting, fd = " << fd;
        if (!Channel::isEnableWrite()){
            sockets::shutdownWrite(fd);
            LOG_TRACE << "TcpConnection::shutdownInLoop() - shutdown(SHUT_WR), fd = " << fd;
        }
    }
}

void TcpConnection::onConnection() {
    ownerLoop->assertInOwnerThread();
    assert(state == connecting);
    setTcpState(connected);
    Channel::enableReadEvent();
    if (tcpHandler)
        tcpHandler->onConnection(shared_from_this());
}

void TcpConnection::handleRead() {
    ownerLoop->assertInOwnerThread();
    if (state != disconnected) {
        ssize_t n = inputBuf.readFd(fd);
        if (n > 0) {
            if (tcpHandler)
                tcpHandler->onMessage(shared_from_this(), &inputBuf);
        } else if (n == 0) { //对端关闭
            LOG_TRACE << "TcpConnection::handleRead() - read = 0, fd = " << fd;
            handleClose();
        } else {
            handleError();
            #ifdef _WIN32
            handleClose();
            #endif
        }
    }
}

void TcpConnection::handleWrite() {
    // 一旦发生错误, handleRead会读到0(若没注册读事件, Channel::handleEvent则直接发生handleClose)
    ownerLoop->assertInOwnerThread();
    if (state == disconnected) 
        return;
    if (Channel::isEnableWrite()) {
        ssize_t n = outputBuf.writeFd(fd);
        if (n > 0) {
            LOG_TRACE << "TcpConnection::handleWrite(), fd = "<< fd << " len = "
                      << n + outputBuf.readableBytes() << ", write = " << n
                      << ", remain = " << outputBuf.readableBytes();
            if (outputBuf.readableBytes() == 0) {
                Channel::disableWriteEvent();
                if (tcpHandler)
                    ownerLoop->queueInLoop(std::bind(&TcpHandler::onWriteComplete, tcpHandler,
                                  shared_from_this()));
                if (state == disconnecting)
                    sockets::shutdownWrite(fd);
            }
        } else {
            if (n != 0) {
                LOG_SYSERR << "TcpConnection::handleWrite()";
                // 若使用select, 因为select通过read/writeSet(handleRead/Write), 判断错误 
                // 若取消读事件, 将不会发生handleRead, 也就不会触发错误处理(所以读写都过程需要错误处理)
                handleClose();
            }
        }
    } else {
        LOG_TRACE << "Connection fd = " << fd << " is disableWriteEvent, no more writing";
    }
}
// TODO
// 1.什么情况会发生handleClose?
    // 1.主动调用(forceClose->handleClose)
    // 2.read == 0 (对端关闭写(EPOLLRDHUP)/本端关闭读)
    // 3.EPOLLHUP & !EPOLLIN (对端RST/本端和对端都shutdown(WR)/本端shutdown(RDWR))
    // 4.write == -1
// 2.handleClose会发生什么?
    // 移除TcpConnection; 先移除Poller的监听队列/channelMap,后TcpServer的connections
    // (有可能执行多次handleClose, 但是只移除一次)
// 3.TcpConnection析构时(TcpServer和用户会持有),或forceClose,才真正close(fd)
// 4.forceClose中disconnecting->disconnected是连续变化,shutdown则会处于disconnecting状态
void TcpConnection::handleClose() {
    ownerLoop->assertInOwnerThread();
    if (state == disconnected) 
        return;
    LOG_TRACE << "TcpConnection::handleClose(), id = " << id << ", fd = " << fd
              << ", " << peerAddr.toStringIpPort();
    removeItself();
    // 防止过早析构(handleClose 后面或许有读写事件)
    ownerLoop->queueInLoop(std::bind(&TcpConnection::tie, shared_from_this()));
    tcpHolder->removeConnection(id);
}

void TcpConnection::handleError() {
    if (state == disconnected)
        return;
    #ifndef _WIN32
    int err = sockets::getSocketError(fd);
    LOG_ERROR << "TcpConnection::handleError(), id = " << id << ", fd = " << fd
            << ", SO_ERROR = " << err << ", "<< peerAddr.toStringIpPort();
    #else
    LOG_SYSERR << "TcpConnection::handleError(), id = " << id << ", fd = " << fd
               << ", " << peerAddr.toStringIpPort();
    #endif
}

void TcpConnection::removeItself() {
    ownerLoop->assertInOwnerThread();
    setTcpState(disconnected);
    if (tcpHandler)
        tcpHandler->onClose(shared_from_this());
    Channel::remove(); 
}

void TcpConnection::forceClose() {
    if (state == connected || state == disconnecting) {
        // 若runInLoop, 则forceClose有可能在发生在handleWrite之前
        // 无论runInLoop还是queueInLoop, 都移除监听事件和close, 导致可能会丢失数据
        // ownerLoop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
        ownerLoop->runInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceClose(int64_t delayMs) {
    if (state == connected || state == disconnecting)
        ownerLoop->addTimer(delayMs, std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
}

void TcpConnection::forceCloseInLoop() {
    ownerLoop->assertInOwnerThread();
    if (state == connected || state == disconnecting) {
        setTcpState(disconnecting);
        handleClose();
        sockets::close(fd);
        fd = -fd - 1;
    }
}

void TcpConnection::enableRead() {
    ownerLoop->runInLoop(std::bind(&TcpConnection::enableReadInLoop, this));
}
void TcpConnection::disableRead() {
    ownerLoop->runInLoop(std::bind(&TcpConnection::disableReadInLoop, this));
}
void TcpConnection::enableReadInLoop() {
    ownerLoop->assertInOwnerThread();
    if (!isRead || !Channel::isEnableRead()) {
        Channel::enableReadEvent();
        isRead = true;
    }
}
void TcpConnection::disableReadInLoop() {
    ownerLoop->assertInOwnerThread();
    if (isRead || Channel::isEnableRead()) {
        Channel::disableReadEvent();
        isRead = false;
    }
}
void TcpConnection::setTcpNoDelay(bool on) { sockets::setTcpNoDelay(fd, on); }

int TcpConnection::setSockOpt(int level, int optname, const void *optval, socklen_t optlen) {
    return sockets::setsockopt(fd, level, optname, optval, optlen);
}
int TcpConnection::getsockopt(int level, int optname, void *optval, socklen_t *optlen) {
    return sockets::getsockopt(fd, level, optname, optval, optlen);
}

Timestamp TcpConnection::getPollTime() { return ownerLoop->getPollTime(); }