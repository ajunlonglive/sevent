#include "sevent/net/http/HttpResponseCodec.h"
#include "sevent/base/Logger.h"
#include "sevent/net/http/HttpParser.h"
#include "sevent/net/http/HttpResponse.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

bool HttpResponseCodec::onConnection(const TcpConnection::ptr &conn, std::any &msg) {
    conn->setContext("HttpParser", HttpParser(false));
    return true;
}

bool HttpResponseCodec::onMessage(const TcpConnection::ptr &conn, std::any &msg) {
    // Buffer *buf = conn->getInputBuf();
    Buffer *buf;
    try {
        buf = any_cast<Buffer *>(msg);
        if (buf == nullptr) {
            LOG_ERROR << "HttpResponseCodec::onMessage(), Buffer* = nullptr";
            return false;
        }
    } catch (std::bad_any_cast&) {
        LOG_FATAL << "HttpResponseCodec::onMessage() - bad_any_cast, msg should be Buffer*";
    }
    bool isContinue = true;
    vector<HttpResponse> responseList;
    HttpParser *parser = any_cast<HttpParser>(&(conn->getContext("HttpParser")));
    while (buf->readableBytes() > 0 && isContinue) {
        size_t parsed= parser->execute(buf->peek(), buf->readableBytes());
        if(conn->getContext("onClose").has_value() && !parser->isKeepAlive())
            parser->execute(buf->peek(), 0); // 处理EOF
        if (parser->isComplete()) {
            buf->retrieve(parsed);
            shared_ptr<HttpParser> p(new HttpParser(false));
            p->swap(*parser);
            responseList.emplace_back(std::move(p));
            // HttpResponse req(std::move(p));
            // msg = req; // 传递response
        } else {
            if (parser->getErr() != 0){
                LOG_ERROR << "HttpResponseCodec::onMessage() - parser err, "
                        << parser->getErrName() << ", " << parser->getErrDesc();
                conn->shutdown();
            }
            // incomplete
            parser->reset();
            isContinue = false;
            // return false;
        }
    }
    if (responseList.size() > 0)
        msg = std::move(responseList);
    return true;
}
bool HttpResponseCodec::onClose(const TcpConnection::ptr &conn, std::any &msg) {
    HttpParser *parser = any_cast<HttpParser>(&(conn->getContext("HttpParser")));
    if (parser->isParsing()) {
        LOG_TRACE << "HttpResponseCodec::onClose(), on eof";
        conn->setContext("onClose", true);
        TcpPipeline::invoke(&PipelineHandler::onMessage, conn, msg, this);
    }
    return true;
}