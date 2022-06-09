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

const size_t bufSize = 64 * 1024;
class DownloadHandler : public TcpHandler {
public:
    DownloadHandler(const string &file) : filename(file) {}
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();

        conn->setHighWaterMark(bufSize); // 64KB
        FILE *fp = fopen(filename.c_str(), "rb");
        if (fp) {
            conn->setContext(filename, fp);
            size_t n = sendFile(fp, conn);
            if (n <= 0) {
                LOG_INFO << "onConnection finished";
        }
        } else {
            conn->shutdown();
            LOG_SYSERR << "DownloadHandler - fopen failed";        
        }
    }
    void onWriteComplete(const TcpConnection::ptr &conn) {
        FILE *fp = std::any_cast<FILE*>(conn->getContext(filename));
        size_t n = sendFile(fp, conn);
        if (n <= 0) {
            LOG_INFO << "onWriteComplete finished";
        }
    } 
    void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {
            // LOG_INFO << "onHighWaterMark, curHight = " << curHight;
    }

    void onClose(const TcpConnection::ptr &conn) {
        FILE *fp = std::any_cast<FILE*>(conn->getContext(filename));
        if (fp)
            fclose(fp);
    }
public:
    size_t sendFile(FILE *fp, const TcpConnection::ptr &conn) {
        char *buf = static_cast<char*>(malloc(bufSize)); // 栈上分配windows上会溢出
        size_t n = fread(buf, 1, bufSize, fp);
        if (n > 0) {
            conn->send(buf, n);
        } else {
            fclose(fp);
            conn->removeContext(filename);
            conn->shutdown();
        }
        free(buf);
        return n;
    }
private:
    const string filename;
};
// 每次发送64KB, 直到发完
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12345);
    string filename = "./b.log";
    DownloadHandler handler(filename);
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();

    return 0;
}