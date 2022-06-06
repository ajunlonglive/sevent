#include "sevent/net/http/HttpParser.h"

#include "sevent/base/CommonUtil.h"
#include "sevent/base/Logger.h"
#include <assert.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

http_parser_settings HttpParser::settings;
namespace {
const string nullStr = "";
int on_message_begin(http_parser *parser) {
    return static_cast<HttpParser *>(parser->data)->on_message_begin(parser);
}
int on_url(http_parser *parser, const char *at, size_t length) {
    return static_cast<HttpParser *>(parser->data)->on_url(parser, at, length);
}
int on_status(http_parser *parser, const char *at, size_t length) {
    return static_cast<HttpParser *>(parser->data)->on_status(parser, at, length);
}
int on_header_field(http_parser *parser, const char *at, size_t length) {
    return static_cast<HttpParser *>(parser->data)->on_header_field(parser, at, length);
}
int on_header_value(http_parser *parser, const char *at, size_t length) {
    return static_cast<HttpParser *>(parser->data)->on_header_value(parser, at, length);
}
int on_headers_complete(http_parser *parser) {
    return static_cast<HttpParser *>(parser->data)->on_headers_complete(parser);
}
int on_body(http_parser *parser, const char *at, size_t length) {
    return static_cast<HttpParser *>(parser->data)->on_body(parser, at, length);
}
int on_message_complete(http_parser *parser) {
    return static_cast<HttpParser *>(parser->data)->on_message_complete(parser);
}
int on_chunk_header(http_parser *parser) {
    return static_cast<HttpParser *>(parser->data)->on_chunk_header(parser);
}
int on_chunk_complete(http_parser *parser) {
    return static_cast<HttpParser *>(parser->data)->on_chunk_complete(parser);
}
class InitHttpSetting {
public:
    InitHttpSetting() {
        http_parser_settings_init(HttpParser::getSetting());
        HttpParser::getSetting()->on_message_begin = ::on_message_begin;
        HttpParser::getSetting()->on_url = ::on_url;
        HttpParser::getSetting()->on_status = ::on_status;
        HttpParser::getSetting()->on_header_field = ::on_header_field;
        HttpParser::getSetting()->on_header_value = ::on_header_value;
        HttpParser::getSetting()->on_headers_complete = ::on_headers_complete;
        HttpParser::getSetting()->on_body = ::on_body;
        HttpParser::getSetting()->on_message_complete = ::on_message_complete;
        HttpParser::getSetting()->on_chunk_header = ::on_chunk_header;
        HttpParser::getSetting()->on_chunk_complete = ::on_chunk_complete;
    }
};
InitHttpSetting initHttpSetting;
}

bool HttpParser::CaseInsensitiveLess::operator()(const string& lhs, const string& rhs) const {
    return CommonUtil::stricmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpParser::HttpParser(bool isrequest) : request(isrequest), state(ready), parsed(0) {
    if (isrequest)
        urlFields.resize(UF_MAX);
    http_parser_init(&parser, isrequest ? HTTP_REQUEST : HTTP_RESPONSE);
    parser.data = this;
}

size_t HttpParser::execute(const char *data, size_t len) {
    state = parsing;
    http_parser_pause(&parser, 0);
    size_t count = http_parser_execute(&parser, &settings, data, len);
    parsed += count;
    return count;
}

int HttpParser::on_message_begin(http_parser *) { return 0; };
int HttpParser::on_url(http_parser *p, const char *at, size_t length) {
    struct http_parser_url u;
	memset(&u, 0, sizeof(u));
	int ret = http_parser_parse_url(at, length, 0, &u);
    if (ret != 0) {
        LOG_ERROR << "HttpParser::on_url() failed, url = " << at;
        return 1;
    } else {
        unsigned int i;
        if (urlFields.size() >= UF_MAX) {
            for (i = 0; i < UF_MAX; i++) {
                if ((u.field_set & (1 << i)) == 0)
                    continue;
                urlFields[i].assign(at + u.field_data[i].off, u.field_data[i].len);
                if (i == UF_QUERY)
                    parseQueryParam(urlFields[i]);
            }
        }
    }
    return 0;
}
int HttpParser::on_status(http_parser *, const char *at, size_t length) { return 0; }
int HttpParser::on_header_field(http_parser *, const char *at, size_t length) {
    fieldTmp.assign(at, length);
    return 0;
}
int HttpParser::on_header_value(http_parser *, const char *at, size_t length) {
    headers[fieldTmp].assign(at, length);
    return 0;
}
int HttpParser::on_headers_complete(http_parser *) { return 0; }
int HttpParser::on_body(http_parser *, const char *at, size_t length) {
    body.append(at, length);
    return 0;
}
int HttpParser::on_message_complete(http_parser *p) { 
    state = completed;
    LOG_TRACE << "HttpParser::on_message_complete ";
    http_parser_pause(p, 1); // FIXME ?
    return 0;
}
int HttpParser::on_chunk_header(http_parser *) { return 0; }
int HttpParser::on_chunk_complete(http_parser *) { return 0; }
void HttpParser::http_parser_set_max_header_size(uint32_t size) {
    ::http_parser_set_max_header_size(size);
}

void HttpParser::parseQueryParam(const string &str) {
    // p=1&s=2, FIXME: "a&d=1" -> {'a&d', '1'}
    size_t length = str.length();
    size_t pos = 0;
    do {
        size_t begin = pos;
        pos = str.find('=', pos);
        if(pos == std::string::npos)
            break;
        string key = str.substr(begin, pos - begin);
        if (++pos >= length)
            break;
        begin = pos;
        pos = str.find('&', pos);
        string value = str.substr(begin, pos - begin);
        if (key.length() > 0)
            params[key] = std::move(value);
        if(pos == std::string::npos)
            break;
        ++pos;
    } while (true);

}

int HttpParser::isKeepAlive() { return http_should_keep_alive(&parser); }
unsigned int HttpParser::getErr() { 
    // err = 31, parser is paused
    if (state == completed)
        return 0;
    return parser.http_errno; 
}
const char *HttpParser::getErrName() {
    return http_errno_name(http_errno(parser.http_errno));
}
const char *HttpParser::getErrDesc() {
    return http_errno_description(http_errno(parser.http_errno));
}

string &HttpParser::getHeader(const string &key) {
    return headers[key];
}
string &HttpParser::getParam(const string &key) {
    return params[key];
}

const string &HttpParser::getSchema() const { 
    return request ? urlFields[UF_SCHEMA] : nullStr;
}
const string &HttpParser::getHost() const { 
    return request ? urlFields[UF_HOST] : nullStr;
}
const string &HttpParser::getPort() const { 
    return request ? urlFields[UF_PORT] : nullStr;
}
const string &HttpParser::getPath() const { 
    return request ? urlFields[UF_PATH] : nullStr;
}
const string &HttpParser::getQuery() const { 
    return request ? urlFields[UF_QUERY] : nullStr;
}
const string &HttpParser::getFragment() const { 
    return request ? urlFields[UF_FRAGMENT] : nullStr;
}
const string &HttpParser::getUserInfo() const {
    return request ? urlFields[UF_USERINFO] : nullStr;
}

void HttpParser::setBody(std::string val) { body = std::move(val); }
void HttpParser::setHeader(const std::string &key, std::string val) {
    headers[key] = std::move(val);
}

void HttpParser::reset() {
    // state 保持不变;
    parsed = 0;
    body.empty();
    fieldTmp.empty();
    urlFields.empty();
    headers.empty();
    params.empty();
    http_parser_init(&parser, request ? HTTP_REQUEST : HTTP_RESPONSE); // 自动保存data(this)
}

void HttpParser::swap(HttpParser &other) {
    std::swap(request, other.request);
    std::swap(state, other.state);
    std::swap(parsed, other.parsed);
    body.swap(other.body);
    fieldTmp.swap(other.fieldTmp);
    urlFields.swap(other.urlFields);
    headers.swap(other.headers);
    params.swap(other.params);
    std::swap(parser, other.parser);
    parser.data = this; // this->parser.data保持不变, 永远指向自己(this)
    other.getParser()->data = &other;
}

void HttpParser::copy(const HttpParser &other) {
    request = other.request;
    state = other.state;
    parsed = other.parsed;
    body = other.body;
    fieldTmp = other.fieldTmp;
    urlFields = other.urlFields;
    headers = other.headers;
    params = other.params;
    parser = other.parser;
    parser.data = this;
}
void HttpParser::copy(HttpParser &&other) {
    request = other.request;
    state = other.state;
    parsed = other.parsed;
    body = std::move(other.body);
    fieldTmp = std::move(other.fieldTmp);
    urlFields = std::move(other.urlFields);
    headers = std::move(other.headers);
    params = std::move(other.params);
    parser = other.parser;
    parser.data = this;
}
HttpParser::HttpParser(const HttpParser &other) { copy(other); }
HttpParser::HttpParser(HttpParser &&other) { copy(std::move(other)); }
HttpParser &HttpParser::operator=(const HttpParser &other) {
    copy(other);
    return *this;
}
HttpParser &HttpParser::operator=(HttpParser &&other) {
    copy(std::move(other));
    return *this;
}

std::string HttpParser::toString() {
    string msg;
    msg.reserve(1024);
    // method/status
    if (request)
        msg += http_method_str(http_method(parser.method));
    else
        msg += http_status_str(http_status(parser.status_code));
    // version
    msg +=  " HTTP/"+ to_string(parser.http_major) + "." + to_string(parser.http_minor);
    // url
    if (request) {
        msg += "\nurl:";
        msg += "\n\tschema:" + urlFields[UF_SCHEMA];
        msg += "\n\thost:" + urlFields[UF_HOST];
        msg += "\n\tport:" + urlFields[UF_PORT];
        msg += "\n\tpath:" + urlFields[UF_PATH];
        msg += "\n\tquery:" + urlFields[UF_QUERY];
        msg += "\n\tfragment:" + urlFields[UF_FRAGMENT];
        msg += "\n\tuserinfo:" + urlFields[UF_USERINFO];
    }
    // headers
    msg += "\nheaders:";
    for (auto &item : headers) {
        msg += "\n\t" + item.first + ":" + item.second;
    }
    // body
    msg += "\nbody:\n\t";
    msg += body;
    return msg;
}