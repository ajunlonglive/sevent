#include "sevent/net/http/HttpResponseCodec.h"
#include "sevent/base/Logger.h"
#include "sevent/net/http/HttpParser.h"
#include "sevent/net/http/HttpResponse.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

HttpResponseCodec::HttpResponseCodec() : HttpCodec(false) {}

void HttpResponseCodec::handleMessage(vector<unique_ptr<HttpParser>> &&parserList, std::any &msg) {
    vector<HttpResponse> responseList;
    for (unique_ptr<HttpParser> &item : parserList) {
        // responseList.push_back(HttpResponse(std::move(item)));
        responseList.emplace_back(std::move(item));
    }
    msg = std::move(responseList);
}

bool HttpResponseCodec::onClose(const TcpConnection::ptr &conn, std::any &msg) {
    HttpParser *parser = std::any_cast<HttpParser>(&(conn->getContext("HttpParser")));
    if (parser->isParsing()) {
        LOG_TRACE << "HttpResponseCodec::onClose(), on eof";
        conn->setContext("onClose", true);
        // 从pipeline头开始, 执行onMessage调用链
        PipelineHandler *handler = this->getPipeLine()->getHandlers()->front();
        TcpPipeline::invoke(&PipelineHandler::onMessage, conn, conn->getInputBuf(), handler);
    }
    return true;
}