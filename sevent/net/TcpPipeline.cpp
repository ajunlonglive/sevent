#include "sevent/net/TcpPipeline.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpPipeline &TcpPipeline::addLast(PipelineHandler *next) {
    PipelineHandler *prev = handlers.back();
    if (prev) {
        prev->setNextHandler(next);
        next->setPrevHandler(prev);
    }
    handlers.push_back(next);
    return *this;
}

void TcpPipeline::onConnection(const TcpConnection::ptr &conn) {
    std::any msg;
    invoke(&PipelineHandler::onConnection, conn, msg);
}
void TcpPipeline::onClose(const TcpConnection::ptr &conn) {
    std::any msg;
    invoke(&PipelineHandler::onClose, conn, msg);
}
void TcpPipeline::onWriteComplete(const TcpConnection::ptr &conn) {
    std::any msg;
    invoke(&PipelineHandler::onWriteComplete, conn, msg);
}  
void TcpPipeline::onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {
    std::any msg = curHight;
    invoke(&PipelineHandler::onHighWaterMark, conn, msg);
}

void TcpPipeline::onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
    std::any msg = std::make_any<Buffer *>(buf);
    invoke(&PipelineHandler::onMessage, conn, msg);
}

void TcpPipeline::invoke(handlerFunc1 func, const TcpConnection::ptr &conn) {
    PipelineHandler *handler = handlers.front();
    bool isNext = false;
    if (handler) {
        do {
            isNext = (handler->*func)(conn);
            handler = handler->nextHandler();
        } while (isNext && handler);
    }
}
void TcpPipeline::invoke(handlerFunc2 func, const TcpConnection::ptr &conn, std::any &msg) {
    PipelineHandler *handler = handlers.front();
    bool isNext = false;
    if (handler) {
        do {
            isNext = (handler->*func)(conn, msg);
            handler = handler->nextHandler();
        } while (isNext && handler);
    }
}

PipelineHandler::PipelineHandler() : prev(nullptr), next(nullptr) {}

void PipelineHandler::onError(const TcpConnection::ptr &conn, std::any &msg) {
    bool isNext = false;
    PipelineHandler *handler = this;
    do {
        isNext = handler->handleError(conn, msg);
        handler = handler->nextHandler();
    } while (isNext && handler);
}

void PipelineHandler::write(const TcpConnection::ptr &conn, std::any &&msg, size_t size) {
    write(conn, msg, size);
}
void PipelineHandler::write(const TcpConnection::ptr &conn, std::any &msg, size_t size) {
    // 从当前handler往前传递msg
    bool isNext = false;
    PipelineHandler *handler = this;
    do {
        isNext = handler->handleWrite(conn, msg, size);
        handler = handler->prevHandler();
    } while (isNext && handler);
    // 发送, 处理完后, msg类型只能为string/Buffer/void*
    if (isNext && msg.has_value()) {
        const type_info &t = msg.type();
        if (t == typeid(string)) {
            conn->send(any_cast<string&>(msg));
        } else if (t == typeid(Buffer)) {
            conn->send(any_cast<Buffer&>(msg));
        } else if (t == typeid(Buffer*)) {
            conn->send(any_cast<Buffer>(&msg));
        } else if (t == typeid(void*)) {
            conn->send(any_cast<void>(&msg), size);
        } else {
            //onError? TODO
        }
    }
}