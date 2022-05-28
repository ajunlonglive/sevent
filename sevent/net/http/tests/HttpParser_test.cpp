#include <iostream>
#include <string.h>
#include "sevent/net/http/HttpParser.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using namespace sevent::net::http;

const char *buf1 = "GET http://a%12:b!&*$@hypnotoad.org:1234/toto?p=1&s=2 HTTP/1.1\r\n"
                   "Connection: Upgrade\r\n"
                   "User-agent: Mozilla/1.1N\r\n"
                   "Content-Length: 15\r\n"
                   "\r\n"
                   "sweet post body";
const char *buf2 = "POST /two_chunks_mult_zero_end HTTP/1.1\r\n"
                   "Transfer-Encoding: chunked\r\n"
                   "\r\n"
                   "5\r\nhello\r\n"
                   "6\r\n world\r\n"
                   "000\r\n"
                   "\r\n";
const char *buf3 = "HTTP/1.1 301 Moved Permanently\r\n"
                   "Location: http://www.google.com/\r\n"
                   "Content-Type: text/html; charset=UTF-8\r\n"
                   "Date: Sun, 26 Apr 2009 11:11:49 GMT\r\n"
                   "Expires: Tue, 26 May 2009 11:11:49 GMT\r\n"
                   "X-$PrototypeBI-Version: 1.6.0.3\r\n" /* $ char in header field */
                   "Cache-Control: public, max-age=2592000\r\n"
                   "Server: gws\r\n"
                   "Content-Length:  219  \r\n"
                   "\r\n"
                   "<HTML><HEAD><meta http-equiv=\"content-type\" "
                   "content=\"text/html;charset=utf-8\">\n"
                   "<TITLE>301 Moved</TITLE></HEAD><BODY>\n"
                   "<H1>301 Moved</H1>\n"
                   "The document has moved\n"
                   "<A HREF=\"http://www.google.com/\">here</A>.\r\n"
                   "</BODY></HTML>\r\n";
const char *buf4 = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/plain\r\n"
                   "Transfer-Encoding: chunked\r\n"
                   "\r\n"
                   "25  \r\n"
                   "This is the data in the first chunk\r\n"
                   "\r\n"
                   "1C\r\n"
                   "and this is the second one\r\n"
                   "\r\n"
                   "0  \r\n"
                   "\r\n";

const char *arrReq[] = {buf1, buf2};
const char *arrRes[] = {buf3, buf4};
void parse(bool isRequest, const char *arr[], int size) {
    static int count = 0;
    for (int i = 0; i < size; ++i) {
        printf("buf%d:==================================\n", ++count);
        HttpParser parser(isRequest);
        {
            HttpParser p(isRequest);
            p.swap(parser); // 测试swap
        }
        const char *buf = arr[i];
        size_t parsed = parser.execute(buf, strlen(buf));
        cout<<"len = " << strlen(buf) << ", parsed = " << parsed <<endl;
        cout<< parser.toString() <<endl;
    }
}

int main(){
    parse(true, arrReq, sizeof(arrReq)/sizeof(arrReq[0]));
    parse(false, arrRes, sizeof(arrRes)/sizeof(arrRes[0]));
    return 0;
}