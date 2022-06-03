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
    HttpResponse();
    HttpResponse(HttpVersion version, HttpStatus status);
    HttpResponse(std::shared_ptr<HttpParser> &&p);
    // 解析response
    int isKeepAlive();
    unsigned short httpMajor() const;
    unsigned short httpMinor() const;
    const std::string &getBody() const;
    const std::string &getHeader(const std::string &key) const;
    // 深拷贝
    HttpResponse duplicate(); 

    // 构建response
    void setHttpVersion(HttpVersion version) { httpVersion = version; }
    void setHttpStatus(HttpStatus status) { httpStatus = status; }
    void setBody(std::string body);
    void setHeader(const std::string &key, std::string val);
    // void setKeepAlive(bool isKeepAlive) { keepAlive = isKeepAlive; }
    // 构建response, FIXME 不完善
    std::string buildString();

    // for test
    std::string toString();

private:
    std::shared_ptr<HttpParser> parser; // std::any不能保存unique_ptr, 拷贝构造已删除
    HttpVersion httpVersion;
    HttpStatus httpStatus;
};

} // namespace http
} // namespace net
} // namespace sevent

#endif