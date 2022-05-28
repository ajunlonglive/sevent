#include "sevent/net/http/http.h"
#include "sevent/net/http/http-parser/http_parser.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;


const char *http::methodToString(http::HttpMethod v) {
    return http_method_str(http_method(static_cast<unsigned int>(v)));
}
const char *http::statusdToString(http::HttpStatus v) {
    return http_status_str(http_status(static_cast<unsigned int>(v)));
}

