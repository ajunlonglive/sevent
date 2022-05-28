#ifndef SEVENT_NET_HTTPREQUEST_H
#define SEVENT_NET_HTTPREQUEST_H

#include "sevent/net/http/http.h"
#include <memory>
#include <string>

namespace sevent {
namespace net {
namespace http {

class HttpParser;
// 浅拷贝shared_ptr<HttpParser>
class HttpRequest {
public:
    HttpRequest() = default;
    HttpRequest(std::shared_ptr<HttpParser> p);
    int isKeepAlive();
    unsigned short httpMajor() const;
    unsigned short httpMinor() const;
    HttpMethod getMethod() const;
    // schema ://[userinfo@]host[:port]/path/.../[?query-string][#fragment]
    const std::string &getSchema() const;
    const std::string &getHost() const;
    const std::string &getPath() const;
    const std::string &getQuery() const;
    const std::string &getFragment() const;
    const std::string &getUserInfo() const;
    const std::string &getBody() const;
    const std::string &getHeader(const std::string &key) const;
    const std::string &getParam(const std::string &key) const;
    HttpRequest duplicate(); // 深拷贝
    // for test
    std::string toString();

private:
    std::shared_ptr<HttpParser> parser;
};
} // namespace http
} // namespace net
} // namespace sevent

#endif