#include "sevent/net/http/HttpRequestCodec.h"
#include "sevent/base/Logger.h"
#include "sevent/net/http/HttpParser.h"
#include "sevent/net/http/HttpRequest.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

bool HttpRequestCodec::onConnection(const TcpConnection::ptr &conn, std::any &msg) {
    conn->setContext("HttpParser", HttpParser(true));
    return true;
}

bool HttpRequestCodec::onMessage(const TcpConnection::ptr &conn, std::any &msg) {
    // Buffer *buf = conn->getInputBuf();
    Buffer *buf;
    try {
        buf = any_cast<Buffer *>(msg);
        if (buf == nullptr) {
            LOG_ERROR << "HttpRequestCodec::onMessage(), Buffer* = nullptr";
            return false;
        }
    } catch (std::bad_any_cast&) {
        LOG_FATAL << "HttpRequestCodec::onMessage() - bad_any_cast, msg should be Buffer*";
    }
    HttpParser *parser = any_cast<HttpParser>(&(conn->getContext("HttpParser")));
    // TODO 现在每次处理并传递1条
    size_t parsed= parser->execute(buf->peek(), buf->readableBytes());
    if (parser->isComplete()) {
        buf->retrieve(parsed);
        shared_ptr<HttpParser> p(new HttpParser(true));
        p->swap(*parser);
        HttpRequest req(std::move(p));
        msg = std::move(req); // 传递request
    } else {
        if (parser->getErr() != 0){
            LOG_ERROR << "HttpRequestCodec::onMessage() - parser err, "
                    << parser->getErrName() << ", " << parser->getErrDesc();
            conn->shutdown();
        }
        // incomplete
        parser->reset();
        return false;
    }
    return true;
}