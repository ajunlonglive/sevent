#include "sevent/net/http/HttpRequest.h"
#include "sevent/net/http/HttpParser.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

HttpRequest::HttpRequest(shared_ptr<HttpParser> p) : parser(std::move(p)) {}

int HttpRequest::isKeepAlive() { return http_should_keep_alive(parser->getParser()); }
unsigned short HttpRequest::httpMajor() const { return parser->httpMajor(); }
unsigned short HttpRequest::httpMinor() const { return parser->httpMinor(); }
HttpMethod HttpRequest::getMethod() const { return HttpMethod(parser->getMethod()); }
// schema ://[userinfo@]host[:port]/path/.../[?query-string][#fragment]
const std::string &HttpRequest::getSchema() const { return parser->getSchema();}
const std::string &HttpRequest::getHost() const { return parser->getHost();}
const std::string &HttpRequest::getPath() const { return parser->getPath();}
const std::string &HttpRequest::getQuery() const { return parser->getQuery();}
const std::string &HttpRequest::getFragment() const { return parser->getFragment();}
const std::string &HttpRequest::getUserInfo() const { return parser->getUserInfo();}
const std::string &HttpRequest::getBody() const { return parser->getBody(); }
const std::string &HttpRequest::getHeader(const std::string &key) const {
    return parser->getHeader(key);
}
const std::string &HttpRequest::getParam(const std::string &key) const {
    return parser->getParam(key);
}

HttpRequest HttpRequest::duplicate() {
    HttpRequest request(make_shared<HttpParser>(*parser));
    return request;
}

std::string HttpRequest::toString() {
    string msg;
    msg.reserve(parser->getParsed());
    msg += http::methodToString(parser->getMethod());
    msg += " ";
    if (parser->getSchema().size() != 0)
        msg += parser->getSchema() + "://";
    if (parser->getUserInfo().size() != 0)
        msg += parser->getUserInfo() + "@";
    if (parser->getHost().size() != 0)
        msg += parser->getHost();
    if (parser->getPort().size() != 0)
        msg += ":"+parser->getPort();
    if (parser->getPath().size() != 0)
        msg += parser->getPath();
    if (parser->getQuery().size() != 0)
        msg += "?"+parser->getQuery();
    msg += " HTTP/" + to_string(parser->httpMajor()) + "." +
           to_string(parser->httpMinor()) + "\r\n";
    for (auto &item : parser->getHeaders()) {
        msg += item.first + ": " + item.second + "\r\n";
    }
    msg += parser->getBody();
    return msg;    

}