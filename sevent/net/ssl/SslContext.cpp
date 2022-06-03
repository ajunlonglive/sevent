#include "sevent/net/ssl/SslContext.h"

#include "sevent/base/Logger.h"
#include "sevent/net/Buffer.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <thread>

using namespace std;
using namespace sevent;
using namespace sevent::net;

atomic<bool> SslContext::isInit = false;
namespace {
thread_local char g_errBuf[256] = {0};
}

SslContext::SslContext() : context(createClientCtx()) {}
SslContext::SslContext(const std::string& certFile, const std::string& keyFile)
    : context(createServerCtx()) {
    initServerCertificate(certFile, keyFile);
}

SslContext::~SslContext() { 
    if (context) {
        SSL_CTX_free(context);
        context = nullptr;
    }
}

void SslContext::init() {
    if (!isInit.exchange(true)) {
    #if OPENSSL_VERSION_NUMBER >= 0x10100003L
        if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
            LOG_FATAL << "SslContext::init() - OPENSSL_init_ssl failed";
        }
        ERR_clear_error();
    #else
        OPENSSL_config(NULL);
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    #endif
    }
}

SSL_CTX * SslContext::createClientCtx() {
    init();
    SSL_CTX *clientContext = SSL_CTX_new(TLS_client_method());
    if (clientContext == nullptr)
        LOG_FATAL << "SslContext::clientSslContext() - SSL_CTX_new() failed";
    return clientContext;
}
#pragma GCC diagnostic ignored "-Wold-style-cast"
SSL_CTX * SslContext::createServerCtx() {
    init();
    SSL_CTX *serverContext = SSL_CTX_new(TLS_server_method());
    if (serverContext == nullptr)
        LOG_FATAL << "SslContext::serverSslContext() - SSL_CTX_new() failed";
    SSL_CTX_set_min_proto_version(serverContext, TLS1_1_VERSION);

#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
    SSL_CTX_set_options(serverContext, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
    SSL_CTX_set_options(serverContext, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_TLS_D5_BUG
    SSL_CTX_set_options(serverContext, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
    SSL_CTX_set_options(serverContext, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    SSL_CTX_set_options(serverContext, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

    SSL_CTX_set_options(serverContext, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(serverContext, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
    SSL_CTX_set_mode(serverContext, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
    SSL_CTX_set_mode(serverContext, SSL_MODE_NO_AUTO_CHAIN);
#endif

    SSL_CTX_set_read_ahead(serverContext, 1);
    return serverContext;
}
#pragma GCC diagnostic warning "-Wold-style-cast"

void SslContext::initServerCertificate(const std::string& certFile, const std::string& keyFile) {
    // 加载证书
    if(!SSL_CTX_use_certificate_file(context, certFile.c_str(), SSL_FILETYPE_PEM))
        LOG_FATAL << "SslContext::initServerCertificate() - certificate_file failed, " << certFile;
    // 加载私钥
    if(!SSL_CTX_use_PrivateKey_file(context, keyFile.c_str(), SSL_FILETYPE_PEM))
        LOG_FATAL << "SslContext::initServerCertificate() - privateKey_file failed, " << keyFile;
    // 验证私钥
    if (!SSL_CTX_check_private_key(context))
        LOG_FATAL << "SslContext::initServerCertificate() - privateKey not "
                     "match the publicKey in certificate";
}

void SslContext::clearErr() { ERR_clear_error(); }
const char *SslContext::getErrStr() {
    ERR_error_string_n(ERR_get_error(), g_errBuf, sizeof(g_errBuf));
    return g_errBuf;
}

int SslContext::getSslErr(const SSL *ssl, int ret) {
    int errCode = SSL_get_error(ssl, ret);
    return errCode;
}

/********************************************************************
 *                              SslHandler
 * ******************************************************************/
SslHandler::SslHandler(SSL_CTX *context, bool isClient)
    : client(isClient), ssl(createSSL(context)), status(CONNECTING) {
    rbio = BIO_new(BIO_s_mem());
    wbio = BIO_new(BIO_s_mem());
    SSL_set_bio(ssl, rbio, wbio);
}

SSL *SslHandler::createSSL(SSL_CTX *context) {
    SSL *s = SSL_new(context);
    if (s == nullptr) {
        LOG_FATAL << "SslHandler::createSSL() - SSL_new() failed";
    } else {
        if (!SSL_clear(s)) {
            LOG_FATAL << "SslHandler::createSSL() - SSL_clear() failed";
            s = nullptr;
        } else {
            if (client)
                SSL_set_connect_state(s);
            else
                SSL_set_accept_state(s);
        }
    }
    return s;
}

SslHandler::~SslHandler() {
    if (ssl) {
        LOG_TRACE << "SslHandler::~SslHandler() - SSL_free()";
        SSL_free(ssl); // free ssl and bio
    }
}

SslHandler::Status SslHandler::getSslStatus(int ret) {
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
            LOG_ERROR << "SSL_ERROR_FAILED, code = " << code;
        return SSL_FAIL;
    }
}

SslHandler::Status SslHandler::ssldoHandshake() {
    ERR_clear_error();
    int ret = SSL_do_handshake(ssl);
    SslHandler::Status status = getSslStatus(ret);
    return status;
}


int SslHandler::bioRead(Buffer &buf) {
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
            // if (!BIO_should_retry(wbio))
            //     LOG_TRACE << "SslHandler::bioRead() - err, n = " << n << ", count = " << count;
        }
        LOG_TRACE << "SslHandler::bioRead(), n = " << n << ", count = " << count;
    } while (n > 0);
    return count > 0 ? count : n;
}

int SslHandler::bioWrite(const Buffer &buf) {
    // >0, 成功写入字节数; -1, 没有成功写入; -2, bio不支持该操作; 0, bio=nullptr或dlen<=0
    int n = BIO_write(rbio, buf.readPos(), static_cast<int>(buf.readableBytes()));
    LOG_TRACE << "SslHandler::bioWrite(), n = " << n << ", count = " << buf.readableBytes();
    return n;
}
int SslHandler::sslRead(Buffer &buf) {
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
            // LOG_TRACE << "SSL_read() - err, n = " << n << ", count = " << count;
        }
        LOG_TRACE << "SslHandler::sslRead() , n = " << n << ", count = " << count;
    } while (n > 0);
    return n;
}

int SslHandler::sslWrite(const Buffer &buf) {
    int n = SSL_write(ssl, buf.readPos(), static_cast<int>(buf.readableBytes()));
    LOG_TRACE << "SslHandler::sslWrite() , n = " << n << ", count = " << buf.readableBytes();
    return n;
}

int SslHandler::encrypted(const Buffer &inbuf, Buffer &outbuf) {
    if (!SSL_is_init_finished(ssl)) {
        LOG_TRACE << "SslHandler::encrypted() - SSL_is_init_finished = false";
        return -1;
    }
    ERR_clear_error();
    int ret = sslWrite(inbuf);
    if (ret > 0) {
        bioRead(outbuf);
    }
    Status status = getSslStatus(ret);
    if (status == SSL_FAIL) {
        LOG_ERROR << "SslClientCodec::encrypted() - failed";
        return -2;
    }
    return ret;
}
SslHandler::Status SslHandler::decrypted(Buffer &inbuf, Buffer &outbuf) {
    int n = bioWrite(inbuf);
    if (n <= 0) {
        LOG_TRACE << "SslHandler::decrypted() - bioWrite failed, ret = " << n;
        return SSL_FAIL;
    }
    inbuf.retrieve(static_cast<size_t>(n));
    ERR_clear_error();
    int ret = sslRead(outbuf);
    Status status = getSslStatus(ret);
    return status;
}