#include "sevent/net/TcpPipeline.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpPipeline &TcpPipeline::addLast(PipelineHandler *handler) {
    handler->setNextHandler(handlers.back());
    handlers.push_back(handler);
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

void TcpPipeline::onMessage(const TcpConnection::ptr &conn, Buffer &buf) {
    std::any msg = std::make_any<Buffer &>(buf);
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
void TcpPipeline::onError(const TcpConnection::ptr &conn, std::any &msg) {
    std::any msg;
    invoke(&PipelineHandler::onError, conn, msg);
}
// PipelineHandler::PipelineHandler() : next(nullptr) {}

// void PipelineHandler::invoke(handlerFunc1 func, const TcpConnection::ptr &conn) {
//     bool isNext = (this->*func)(conn);
//     while (isNext && next) {
//         isNext = (next->*func)(conn);
//         next = next->next;
//     }
// }
// void PipelineHandler::invoke(handlerFunc2 func, const TcpConnection::ptr &conn, std::any &msg) {
//     bool isNext = (this->*func)(conn, msg);
//     while (isNext && next) {
//         isNext = (next->*func)(conn, msg);
//         next = next->next;
//     }
// }