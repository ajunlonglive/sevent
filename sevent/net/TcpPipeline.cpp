#include "sevent/net/TcpPipeline.h"
#include "sevent/base/Logger.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

TcpPipeline &TcpPipeline::addLast(PipelineHandler *next) {
    if (next == nullptr) {
        LOG_ERROR << "TcpPipeline::addLast() - PipelineHandler is nullptr";
        return *this;
    }
    PipelineHandler *prev = handlers.back();
    if (prev) {
        prev->setNextHandler(next);
        next->setPrevHandler(prev);
    }
    handlers.push_back(next);
    next->setPipeLine(this);
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
    invoke(func, conn, handler);
}
void TcpPipeline::invoke(handlerFunc2 func, const TcpConnection::ptr &conn, std::any &msg) {
    PipelineHandler *handler = handlers.front();
    invoke(func, conn, msg, handler);
}
// 从指定handler往后传播
void TcpPipeline::invoke(handlerFunc1 func, const TcpConnection::ptr &conn, PipelineHandler *handler) {
    bool isNext = false;
    if (handler) {
        do {
            isNext = (handler->*func)(conn);
            handler = handler->nextHandler();
        } while (isNext && handler);
    }    
}
void TcpPipeline::invoke(handlerFunc2 func, const TcpConnection::ptr &conn, std::any &msg, PipelineHandler *handler) {
    bool isNext = false;
    if (handler) {
        do {
            isNext = (handler->*func)(conn, msg);
            handler = handler->nextHandler();
        } while (isNext && handler);
    }  
}
void TcpPipeline::invoke(handlerFunc2 func, const TcpConnection::ptr &conn, std::any &&msg, PipelineHandler *handler) {
    invoke(func, conn, msg, handler);
}

PipelineHandler::PipelineHandler() : prev(nullptr), next(nullptr), pipeline(nullptr) {}

void PipelineHandler::onError(const TcpConnection::ptr &conn, std::any &msg) {
    TcpPipeline::invoke(&PipelineHandler::handleError, conn, msg, this);
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
    // 发送, 处理完后, msg类型只能为string(*)/Buffer(*)/const char*
    if (isNext && msg.has_value()) {
        const type_info &t = msg.type();
        try{
            if (t == typeid(string)) {
                conn->send(any_cast<string&>(msg));
            } else if (t == typeid(string*)) {
                conn->send(*any_cast<string*>(msg));
            } else if (t == typeid(Buffer*)) {
                conn->send(any_cast<Buffer*>(msg));
            } else if (t == typeid(Buffer)) {
                conn->send(any_cast<Buffer&>(msg));
            } else if (t == typeid(const char*)) {
                conn->send(any_cast<const char*>(msg), size);
            } else {
                //onError? TODO
            }
        } catch (std::bad_any_cast&) {
            LOG_FATAL << "PipelineHandler::write() - bad_any_cast, msg should "
                         "be string/Buffer/const char*";
        }
    }
}