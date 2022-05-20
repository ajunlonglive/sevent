#ifndef SEVENT_NET_TCPHANDLERCONTEXT_H
#define SEVENT_NET_TCPHANDLERCONTEXT_H

#include "sevent/net/TcpHandler.h"
#include <any>
#include <list>
namespace sevent {
namespace net {

class PipelineHandler {
public:
    PipelineHandler();
    PipelineHandler *prevHandler() { return prev; }
    PipelineHandler *nextHandler() { return next; }
    void setPrevHandler(PipelineHandler *prevHandler) { prev = prevHandler; }
    void setNextHandler(PipelineHandler *nextHandler) { next = nextHandler; }

    void onError(const TcpConnection::ptr &conn, std::any &msg);
    void write(const TcpConnection::ptr &conn, std::any &msg, size_t size = 0);
    void write(const TcpConnection::ptr &conn, std::any &&msg, size_t size = 0);

    virtual bool onMessage(const TcpConnection::ptr &conn, std::any &msg) { return true; }
    virtual bool onConnection(const TcpConnection::ptr &conn, std::any &msg) { return true; }
    virtual bool onClose(const TcpConnection::ptr &conn, std::any &msg) { return true; }
    virtual bool onWriteComplete(const TcpConnection::ptr &conn, std::any &msg) { return true; }
    virtual bool onHighWaterMark(const TcpConnection::ptr &conn, std::any &msg) { return true; }
    virtual bool handleError(const TcpConnection::ptr &conn, std::any &msg) { return true; }
    virtual bool handleWrite(const TcpConnection::ptr &conn, std::any &msg, size_t size) { return true; }
    virtual ~PipelineHandler() = default;

private:
    PipelineHandler *prev;
    PipelineHandler *next;
};

class TcpPipeline : public TcpHandler {
public:
    TcpPipeline &addLast(PipelineHandler *handler);
    void onConnection(const TcpConnection::ptr &conn) override;
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) override;
    void onClose(const TcpConnection::ptr &conn) override;
    void onWriteComplete(const TcpConnection::ptr &conn) override;  
    void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) override;

private:
    using handlerFunc1 = bool (PipelineHandler::*)(const TcpConnection::ptr &conn);
    using handlerFunc2 = bool (PipelineHandler::*)(const TcpConnection::ptr &conn, std::any &msg);
    void invoke(handlerFunc1 func, const TcpConnection::ptr &conn);
    void invoke(handlerFunc2 func, const TcpConnection::ptr &conn, std::any &msg);
private:
    std::list<PipelineHandler *> handlers;
};



} // namespace net
} // namespace sevent

#endif