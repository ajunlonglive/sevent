#include "sevent/net/http/HttpCodec.h"
#include "sevent/base/Logger.h"
#include "sevent/net/http/HttpParser.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

HttpCodec::HttpCodec(bool isReq) : isRequest(isReq) {}

bool HttpCodec::onConnection(const TcpConnection::ptr &conn, std::any &msg) {
    conn->setContext("HttpParser", make_any<HttpParser>(isRequest));
    return true;
}

bool HttpCodec::onMessage(const TcpConnection::ptr &conn, std::any &msg) {
    // Buffer *buf = conn->getInputBuf();
    Buffer *buf;
    try {
        buf = any_cast<Buffer *>(msg);
        if (buf == nullptr) {
            LOG_ERROR << "HttpCodec::onMessage(), Buffer* = nullptr";
            return false;
        }
    } catch (std::bad_any_cast&) {
        LOG_FATAL << "HttpCodec::onMessage() - bad_any_cast, msg should be Buffer*";
    }
    bool isContinue = true;
    vector<unique_ptr<HttpParser>> parserList;
    HttpParser *parser = any_cast<HttpParser>(&(conn->getContext("HttpParser")));
    while (buf->readableBytes() > 0 && isContinue) {
        size_t parsed= parser->execute(buf->peek(), buf->readableBytes());
        if(conn->getContext("onClose").has_value() && !parser->isKeepAlive())
            parser->execute(buf->peek(), 0); // 处理EOF
        if (parser->isComplete()) {
            buf->retrieve(parsed);
            HttpParser *p = new HttpParser(isRequest);
            p->swap(*parser);
            parserList.emplace_back(p);
        } else {
            if (parser->getErr() != 0){
                LOG_ERROR << "HttpCodec::onMessage() - parser err, "
                        << parser->getErrName() << ", " << parser->getErrDesc();
                conn->shutdown();
            }
            // incomplete
            parser->reset();
            isContinue = false;
        }
    }
    if (parserList.size() <= 0)
        return false;
    handleMessage(std::move(parserList), msg);
    return true;
}