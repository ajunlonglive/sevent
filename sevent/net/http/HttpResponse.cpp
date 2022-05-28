#include "sevent/net/http/HttpResponse.h"
#include "sevent/net/http/HttpParser.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

HttpResponse::HttpResponse()
    : parser(new HttpParser(false)), httpVersion(HttpVersion::HTTP_1_1),
      httpStatus(HttpStatus::HTTP_STATUS_OK) {}
HttpResponse::HttpResponse(shared_ptr<HttpParser> &&p) : parser(std::move(p)) {}
HttpResponse::HttpResponse(HttpVersion version, HttpStatus status)
    : parser(new HttpParser(false)), httpVersion(version), httpStatus(status) {}

int HttpResponse::isKeepAlive() { return http_should_keep_alive(parser->getParser()); }
unsigned short HttpResponse::httpMajor() const { return parser->httpMajor(); }
unsigned short HttpResponse::httpMinor() const { return parser->httpMinor(); }
const std::string &HttpResponse::getBody() const { return parser->getBody(); }
const std::string &HttpResponse::getHeader(const std::string &key) const {
    return parser->getHeader(key);
}

HttpResponse HttpResponse::duplicate() {
    HttpResponse response(make_shared<HttpParser>(*parser));
    return response;
}

std::string HttpResponse::toString() {
    string msg;
    msg.reserve(parser->getParsed());
    msg += "HTTP/" + to_string(parser->httpMajor()) + "." +
           to_string(parser->httpMinor());
    HttpStatus s = parser->getStatus();
    msg += " " + to_string(static_cast<int>(s)) + " " +http::statusdToString(s) + "\r\n";
    for (auto &item : parser->getHeaders()) {
        msg += item.first + ": " + item.second + "\r\n";
    }
    msg += "\r\n";
    msg += parser->getBody();
    return msg;
}

std::string HttpResponse::buildString() {
    string msg;
    msg.reserve(1024);
    msg += http::versionToString(httpVersion);
    msg += " ";
    msg += http::statusdToString(httpStatus);
    msg += "\r\n";

    for (auto &item : parser->getHeaders()) {
        msg += item.first + ": " + item.second + "\r\n";
    }
    if (parser->getBody().size() != 0) {
        msg += "Content-Length: " + to_string(parser->getBody().size()) + "\r\n";
    }
    msg += "\r\n";
    msg += parser->getBody();
    return msg;
}

void HttpResponse::setBody(std::string body) {
    parser->setBody(std::move(body));
}
void HttpResponse::setHeader(const std::string &key, std::string val) {
    parser->setHeader(key, std::move(val));
}