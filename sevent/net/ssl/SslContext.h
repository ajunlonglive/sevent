#ifndef SEVENT_NET_SSLCONTEXT_H
#define SEVENT_NET_SSLCONTEXT_H

#include "sevent/base/noncopyable.h"
#include <atomic>
#include <memory>
#include <string>
#include <openssl/ssl.h>

extern "C" {
// struct BIO;
// struct SSL;
// struct SSL_CTX;
}

namespace sevent {
namespace net {
class Buffer;
// 一般来说一个clientContext就够用了(多个连接可以共用一个)
class SslContext : noncopyable {
public:
    // for client
    SslContext();
    // for server
    SslContext(const std::string& certFile, const std::string& keyFile);
    ~SslContext();
    SSL_CTX *sslContext() { return context; }
    static const char *getErrStr();
    static void clearErr();
    static int getSslErr(const SSL *ssl, int ret);

private:
    static void init();
    SSL_CTX *createServerCtx();
    SSL_CTX *createClientCtx();
    void initServerCertificate(const std::string& certFile, const std::string& keyFile);

private:
    SSL_CTX *context;
    static std::atomic<bool> isInit;
};

//  https://github.com/darrenjs/openssl_examples
//
//  +------+                                    +-----+
//  |......|--> read(fd) --> BIO_write(rbio) -->|.....|--> SSL_read(ssl)  --> IN
//  |......|                                    |.....|
//  |.sock.|                                    |.SSL.|
//  |......|                                    |.....|
//  |......|<-- write(fd) <-- BIO_read(wbio) <--|.....|<-- SSL_write(ssl) <-- OUT
//  +------+                                    +-----+
//          |                                  |       |                     |
//          |<-------------------------------->|       |<------------------->|
//          |         encrypted bytes          |       |  unencrypted bytes  |

// 对BIO_read/SSL_read/SSL_get_error/SSL_do_handshake等函数的简单封装
class SslHandler {
public:
    enum ConnectStatus {CONNECTING, CONNECTED, DISCONNECTING, DISCONNECTED};
    enum Status { SSL_OK, SSL_WANT, SSL_CLOSE, SSL_FAIL };
public:
    SslHandler(SSL_CTX *context, bool isClient);
    ~SslHandler();

    Status ssldoHandshake();
    int bioRead(Buffer &buf); // 从wbio读取并写入Buffer
    int bioWrite(const Buffer &buf); // 从Buffer读取并写入rbio
    int sslRead(Buffer &buf); 
    int sslWrite(const Buffer &buf);
    Status getSslStatus(int ret);
    ConnectStatus getConnStatus() { return status;}
    void setConnStatus(ConnectStatus s) { status = s; }

    SSL *getSSL() { return ssl; }
    BIO *getRBio() { return rbio; }
    BIO *getWBio() { return wbio; }

    int encrypted(const Buffer &inbuf, Buffer &outbuf);
    Status decrypted(Buffer &inbuf, Buffer &outbuf);
    
    // void setSSL(SSL *s) { ssl = s; }

private:
    SSL *createSSL(SSL_CTX *context);
private:
    bool client;
    SSL *ssl;
    BIO *rbio;
    BIO *wbio;
    ConnectStatus status;
};

} // namespace net
} // namespace sevent

#endif