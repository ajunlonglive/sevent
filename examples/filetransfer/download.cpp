#include <iostream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpHandler.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

class DownloadHandler : public TcpHandler {
public:
    DownloadHandler(const string &file) : filename(file) {}
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        conn->send(readFile());
        conn->shutdown();
        LOG_INFO << "finished";
    }
    void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {
        LOG_INFO << "onHighWaterMark " << curHight;
    }
public:
    string readFile() {
        string content;
        FILE *fp = fopen(filename.c_str(), "rb");
        assert(fp != nullptr);
        if (!fp)
            LOG_SYSERR << "DownloadHandler - fopen failed";
        const int bufSize = 1024 * 1024;
        char *fpBuffer = static_cast<char*>(malloc(bufSize)); //若栈上分配windows上会溢出, linux没问题
        char *buf = static_cast<char*>(malloc(bufSize));
        setvbuf(fp, fpBuffer, _IOFBF, sizeof(fpBuffer));
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
            content.append(buf, n);
        }
        LOG_INFO << "file size = " << content.size() << "bytes";
        fclose(fp);
        free(buf);
        free(fpBuffer);
        return content;
    }
private:
    const string filename;
};
// 一次读完, 一次发送
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);
    string filename = "./a.log";
    DownloadHandler handler(filename);
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();

    return 0;
}