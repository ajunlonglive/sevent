#ifndef SEVENT_NET_HTTPRESPONSE_H
#define SEVENT_NET_HTTPRESPONSE_H

#include "sevent/net/http/http.h"
#include <memory>
#include <string>

namespace sevent {
namespace net {
namespace http {

class HttpParser;
class HttpResponse {
public:
    HttpResponse() = default;
    HttpResponse(std::shared_ptr<HttpParser> p);
    int isKeepAlive();
    unsigned short httpMajor() const;
    unsigned short httpMinor() const;
    const std::string &getBody() const;
    const std::string &getHeader(const std::string &key) const;
    HttpResponse duplicate(); // 深拷贝
    // for test
    std::string toString();

private:
    std::shared_ptr<HttpParser> parser;
};

} // namespace http
} // namespace net
} // namespace sevent

#endif