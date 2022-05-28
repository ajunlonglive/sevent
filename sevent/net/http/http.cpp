#include "sevent/net/http/http.h"
#include "sevent/net/http/http-parser/http_parser.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

namespace {
const char *HttpVersionName[static_cast<int>(HttpVersion::HTTP_SIZE)] = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0"};
}


const char *http::methodToString(http::HttpMethod v) {
    return http_method_str(http_method(static_cast<unsigned int>(v)));
}
const char *http::statusdToString(http::HttpStatus v) {
    return http_status_str(http_status(static_cast<unsigned int>(v)));
}

const char *http::versionToString(http::HttpVersion v) {
    return HttpVersionName[static_cast<int>(v)];
}