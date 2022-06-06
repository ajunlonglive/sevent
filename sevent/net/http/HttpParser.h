#ifndef SEVENT_NET_HTTPPARSER_H
#define SEVENT_NET_HTTPPARSER_H

#include "sevent/net/http/http-parser/http_parser.h"
#include "sevent/net/http/http.h"
#include <map>
#include <string>
#include <vector>
// 判断http报文结束
// 1.content-length
// 2.transfer-encoding:chunked
// 3.connection:close(read = 0, 读到EOF) //"HTTP/1.1 404 Not Found\r\n\r\n"
namespace sevent {
namespace net {
namespace http {
// 参考: https://github.com/nodejs/http-parser
class HttpParser {
public:
    explicit HttpParser(bool isrequest = true);
    HttpParser(const HttpParser &other);
    HttpParser(HttpParser &&other);
    HttpParser &operator=(const HttpParser &other);
    HttpParser &operator=(HttpParser &&other);
    ~HttpParser() = default;

    // 返回成功解析的字节数, 并设置err
    size_t execute(const char *data, size_t len);
    bool isRequest() { return request; }
    bool isComplete() { return state == completed; }
    bool isParsing() { return state == parsing; }
    int isKeepAlive();
    void reset();
    void swap(HttpParser &other);
    unsigned short httpMajor() const { return parser.http_major; }
    unsigned short httpMinor() const { return parser.http_minor; }
    unsigned int upgrade() const { return parser.upgrade; }
    // response only
    HttpStatus getStatus() const { return HttpStatus(parser.status_code); }
    // request only
    HttpMethod getMethod() const { return HttpMethod(parser.method); }
    // schema ://[userinfo@]host[:port]/path/.../[?query-string][#fragment]
    const std::string &getSchema() const;
    const std::string &getHost() const;
    const std::string &getPort() const; // 若url上没有port则为""
    const std::string &getPath() const;
    const std::string &getQuery() const;
    const std::string &getFragment() const;
    const std::string &getUserInfo() const;
    const std::string &getBody() const { return body; }
    std::string &getHeader(const std::string &key);
    std::string &getParam(const std::string &key);

    void setBody(std::string val);
    void setHeader(const std::string &key, std::string val);

    // 成功:0, 失败:非0
    unsigned int getErr();
    const char *getErrName();
    const char *getErrDesc();

    size_t getParsed() { return parsed; }
    http_parser *getParser() { return &parser; }
    static http_parser_settings *getSetting() { return &settings; }

    int on_message_begin(http_parser *);
    int on_url(http_parser *, const char *at, size_t length);
    int on_status(http_parser *, const char *at, size_t length);
    int on_header_field(http_parser *, const char *at, size_t length);
    int on_header_value(http_parser *, const char *at, size_t length);
    int on_headers_complete(http_parser *);
    int on_body(http_parser *, const char *at, size_t length);
    int on_message_complete(http_parser *);
    int on_chunk_header(http_parser *);
    int on_chunk_complete(http_parser *); // TODO
    static void http_parser_set_max_header_size(uint32_t size);

public:
    struct CaseInsensitiveLess {
        bool operator()(const std::string &lhs, const std::string &rhs) const;
    };
    using Map = std::map<std::string, std::string, CaseInsensitiveLess>;
    // for test
    std::string toString();
    Map &getHeaders() { return headers; }
    Map &getParams() { return params; }
private:
    void copy(const HttpParser &other);
    void copy(HttpParser &&other);

protected:
    void parseQueryParam(const std::string &str);

protected:
    enum Status { ready, parsing, completed };
    bool request;
    Status state;
    size_t parsed;
    std::string body;
    std::string fieldTmp;
    std::vector<std::string> urlFields; // rquest
    Map headers;
    Map params;

    http_parser parser;
    static http_parser_settings settings;
};

} // namespace http
} // namespace net
} // namespace sevent

#endif