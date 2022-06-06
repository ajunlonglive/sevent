#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EndianOps.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#include "sevent/net/ssl/SslContext.h"
#include <openssl/err.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

SslContext context;
SslHandler sslHolder(&context);
SSL *ssl = sslHolder.getSSL();
BIO *rbio = sslHolder.getRBio();
BIO *wbio = sslHolder.getWBio();
bool isRun = false;
const char *requestStr = "GET / HTTP/1.1\r\n\r\n";

enum Status { SSL_OK, SSL_WANT, SSL_CLOSE, SSL_FAIL };

Status getSslStatus(int ret) {
    int code = SSL_get_error(ssl, ret);
    switch (code) {
        case SSL_ERROR_NONE:
            LOG_TRACE << "SSL_ERROR_NONE";
            return SSL_OK;
        case SSL_ERROR_WANT_WRITE:
            LOG_TRACE << "SSL_ERROR_WANT_WRITE";
            return SSL_WANT;
        case SSL_ERROR_WANT_READ:
            LOG_TRACE << "SSL_ERROR_WANT_READ";
            return SSL_WANT;
        case SSL_ERROR_ZERO_RETURN:
            LOG_TRACE << "SSL_ERROR_ZERO_RETURN";
            return SSL_CLOSE;
        default:
            LOG_TRACE << "SSL_ERROR_FAILED, code = " << code;
        return SSL_FAIL;
    }
}

int bioRead(Buffer &buf) {
    int n = 0;
    int count = 0;
    do {
        n = BIO_read(wbio, buf.writePos(), static_cast<int>(buf.writableBytes()));
        if (n > 0) {
            count += n;
            buf.advance(n);
            if (buf.writableBytes() == 0)
                buf.ensureSpace(buf.size());
        } else {
            if (!BIO_should_retry(wbio))
                LOG_TRACE << "readBio() - err, n = " << n << ", count = " << count;
        }
        LOG_TRACE << "readBio(), n = " << n << ", count = " << count;
    } while (n > 0);
    return count > 0 ? count : n;
}
int bioWrite(const Buffer &buf) {
    // >0, 成功写入字节数; -1, 没有成功写入; -2, bio不支持该操作; 0, bio=nullptr或dlen<=0
    int n = BIO_write(rbio, buf.readPos(), static_cast<int>(buf.readableBytes()));
    LOG_TRACE << "writeBio(), n = " << n << ", count = " << buf.readableBytes();
    return n;
}

int sslRead(Buffer &buf) {
    int n = 0;
    int count = 0;
    do {
        n = SSL_read(ssl, buf.writePos(), static_cast<int>(buf.writableBytes()));
        if (n > 0) {
            count += n;
            buf.advance(n);
            if (buf.writableBytes() == 0)
                buf.ensureSpace(buf.size());
        } else {
            LOG_TRACE << "SSL_read() - err, n = " << n << ", count = " << count;
        }
        LOG_TRACE << "SSL_read() , n = " << n << ", count = " << count;
    } while (n > 0);
    return n;
}

int sslWrite(const Buffer &buf) {
    int n = SSL_write(ssl, buf.readPos(), static_cast<int>(buf.readableBytes()));
    LOG_TRACE << "SSL_write() , n = " << n << ", count = " << buf.readableBytes();
    return n;
}

// unencrypted -> encrypted 
void write(const TcpConnection::ptr &conn, Buffer &buf) {
    if (!SSL_is_init_finished(ssl)) {
        LOG_TRACE << "write - SSL_is_init_finished = false";
        return ;
    }
    ERR_clear_error();
    int ret = sslWrite(buf);
    Buffer tmpBuf;
    if (ret > 0) {
        bioRead(tmpBuf);
    }
    Status status = getSslStatus(ret);
    if (status == SSL_FAIL) {
        LOG_TRACE << "write - sslWrite failed";
        conn->shutdown();
    }
    conn->send(std::move(tmpBuf));
}

class MyHandler : public TcpHandler{
public:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_TRACE << "connection";
        ERR_clear_error();
        int ret = SSL_do_handshake(ssl);
        Status status = getSslStatus(ret);
        if (status == SSL_WANT) {
            Buffer tmpBuf;
            bioRead(tmpBuf);
            conn->send(std::move(tmpBuf));
        } else if (status == SSL_FAIL) {
                LOG_TRACE << "onConnection - FAIL";
                conn->shutdown();
                return;
        }
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        LOG_TRACE << "onmessage, recv byte = " << buf->readableBytes();
        Buffer tmpBuf;
        size_t remain = buf->readableBytes();
        while (remain > 0) {
            int n = bioWrite(*buf);
            if (n <= 0) {
                LOG_ERROR << "onMessage() - writeBio failed, n = " << n;
                conn->shutdown();
                return;
            }
            remain -= n;
            buf->retrieve(static_cast<size_t>(n));

            // encrypted -> unencrypted
            ERR_clear_error();
            int ret = sslRead(tmpBuf);

            Status status = getSslStatus(ret);
            if (status == SSL_WANT) {
                // handshake/renegotiation ?
                if (!SSL_is_init_finished(ssl)) {
                    Buffer tmpBuf2;
                    bioRead(tmpBuf2);
                    conn->send(std::move(tmpBuf2));
                }
                // 握手完毕
                if (SSL_is_init_finished(ssl) && !isRun) {
                    LOG_TRACE << "\nonMessage() - finished";
                    Buffer b;
                    b.append(requestStr);
                    write(conn, b);
                    isRun = true;
                }
                printf("\n");
                // return;
            } else if (status == SSL_FAIL) {
                LOG_TRACE << "onMessage - FAIL";
                conn->shutdown();
                return;
            }
        }
        printf("%s\n", tmpBuf.peek());
    }
private:

};

// 早期测试的实现, 握手完成后, 发送requestStr = "GET / HTTP/1.1"
// ./SslHandler_test ip port
int main(int argc, char **argv){
    string ip = "127.0.0.1";
    uint16_t port = 12345;
    if (argc > 2) {
        ip = argv[1];
        port = static_cast<uint16_t>(atoi(argv[2]));
    }

    EventLoop loop;
    InetAddress addr(ip, port);
    MyHandler handler;
    TcpClient client(&loop, addr);
    client.setTcpHandler(&handler);

    client.connect();
    loop.loop();
    return 0;
}