#include <iostream>
#include <string.h>
#include "sevent/net/http/HttpParser.h"
#include "sevent/net/http/HttpResponse.h"
#include "sevent/net/http/HttpRequest.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

void testHttpResponse() {
    HttpResponse response(HttpVersion::HTTP_1_1, HttpStatus::HTTP_STATUS_OK);
    response.setHeader("Content-Type", "text/html; charset=UTF-8");
    response.setBody("reponse msg\r\n");
    cout<<response.buildString()<<endl;
}
void testHttpRequest() {
    HttpRequest request(HttpVersion::HTTP_1_1, HttpMethod::HTTP_POST);
    request.setUrl("/chunked_w_content_length");
    request.setHeader("Host", "example.com");
    request.setBody("request msg\r\n");
    cout<<request.buildString()<<endl;
}
int main(){
    testHttpResponse();
    testHttpRequest();
    return 0;
}