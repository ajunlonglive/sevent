#include "sevent/net/http/HttpRequestCodec.h"
#include "sevent/base/Logger.h"
#include "sevent/net/http/HttpParser.h"
#include "sevent/net/http/HttpRequest.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

HttpRequestCodec::HttpRequestCodec() : HttpCodec(true) {}

void HttpRequestCodec::handleMessage(vector<unique_ptr<HttpParser>> &&parserList, std::any &msg) {
    vector<HttpRequest> responseList;
    for (unique_ptr<HttpParser> &item : parserList) {
        // responseList.push_back(HttpRequest(std::move(item)));
        responseList.emplace_back(std::move(item));
    }
    msg = std::move(responseList);
}