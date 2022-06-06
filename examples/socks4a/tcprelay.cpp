#include <iostream>
#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpServer.h"
#include "sevent/net/TcpHandler.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;
using namespace sevent;
using namespace sevent::net;

size_t g_highWaterMark = 1024 * 1024;
TcpConnection::ptr g_nullConnection;

class Tunnel : public TcpHandler {
public:
    Tunnel(EventLoop *loop, const InetAddress &addr, const TcpConnection::ptr &serConn)
     : client(loop, addr), serverConn(serConn) {
        LOG_INFO << "Tunnel " << serverConn->getPeerAddr().toStringIpPort()
                 << " <-> " << addr.toStringIpPort();
        client.setTcpHandler(this);
        serverConn->setHighWaterMark(g_highWaterMark);
    }
    ~Tunnel() {
        // 防止析构后, clientConn(存活时间可能比Tunnel长)访问野指针
        if (clientConn)
            clientConn->setTcpHandler(nullptr);
        LOG_INFO << "~Tunnel"; 
    }

    void connect() { client.connect(); }
    void shutdown() { client.shutdown(); }

private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << "Tunnel as client onConnection";
        conn->setTcpNoDelay(true);
        conn->setHighWaterMark(g_highWaterMark);
        clientConn = conn;
        serverConn->setContext("client", clientConn);
        serverConn->enableRead();
        if (serverConn->getInputBuf()->readableBytes() > 0)
            conn->send(serverConn->getInputBuf()); // 在连接服务端期间, 从客户端收到数据
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        LOG_INFO << "Tunnel as client recv " << buf->readableBytes() << " bytes";
        if (serverConn) {
            serverConn->send(buf);
        } else {
            buf->retrieveAll();
            LOG_ERROR << "onMessage serverConn no exist";
        }
    }
    void onClose(const TcpConnection::ptr &conn) {
        // 当服务端断开连接
        LOG_INFO << "Tunnel as client onClose";
        serverConn->setContext("client", std::any());
        serverConn->shutdown();
        clientConn->setTcpHandler(nullptr);
        clientConn.reset();
    }
    void onWriteComplete(const TcpConnection::ptr &conn) {
        if (!serverConn->isReading()) {
            LOG_INFO << "Tunnel as client onWriteComplete, enable server read";
            serverConn->enableRead();
        }
    }
    void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {
        // 从客户端收到数据, 转发给服务端到达高水位(所以应该停止读客户端)
        LOG_INFO << "Tunnel as client onHighWaterMark, disable server read, curHight = " << curHight;
        serverConn->disableRead();
    }

private:
    TcpClient client;
    TcpConnection::ptr clientConn; // 作为client连接服务端
    TcpConnection::ptr serverConn; // 作为server接收客户端
};

class TcpRelayHandler : public TcpHandler {
public:
    TcpRelayHandler(EventLoop *loop, const InetAddress &addr) : loop(loop), addr(addr) {}
private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << "Tunnel as server onConnection";
        conn->setTcpNoDelay(true);
        conn->disableRead(); // 停止读客户端, 直到tunnel成功连接服务端
        shared_ptr<Tunnel> tunnel(new Tunnel(loop, addr, conn));
        tunnel->connect();
        tunnels[conn->getId()] = tunnel;
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        LOG_INFO << "Tunnel as server recv " << buf->readableBytes() << " bytes";
        const TcpConnection::ptr &clientConn = getClientConntion(conn);
        if (clientConn)
            clientConn->send(buf);
    }
    void onClose(const TcpConnection::ptr &conn) {
        // 当客户端断开连接
        LOG_INFO << "Tunnel as server onClose";
        tunnels[conn->getId()]->shutdown(); // 关闭服务端连接
        tunnels.erase(conn->getId());
    }
    void onWriteComplete(const TcpConnection::ptr &conn) {
        const TcpConnection::ptr &clientConn = getClientConntion(conn);
        if (clientConn) {
            if (!clientConn->isReading()) {
                LOG_INFO << "Tunnel as server onWriteComplete, enable server read";
                clientConn->enableRead();
            }
        }
    }
    void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {
        // 从服务端收到数据, 转发给客户端到达高水位(所以应该停止读服务端)
        LOG_INFO << "Tunnel as server onHighWaterMark, disable server read, curHight = " << curHight;
        const TcpConnection::ptr &clientConn = getClientConntion(conn);
        if (clientConn)
            clientConn->disableRead();
    }
    const TcpConnection::ptr &getClientConntion(const TcpConnection::ptr &conn) {
        std::any &a = conn->getContext("client");
        if (a.has_value()) {
            TcpConnection::ptr &clientConn= any_cast<TcpConnection::ptr&>(a);
            return clientConn;
        }
        return g_nullConnection;
    }
private:
    EventLoop *loop;
    const InetAddress &addr;
    unordered_map<int64_t, shared_ptr<Tunnel>> tunnels;
};

// 客户端 <---> tunnel <---> 服务端
int main(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    InetAddress addr("127.0.0.1", 12345); // 服务端地址
    TcpServer server(&loop, 12346);    
    TcpRelayHandler handler(&loop, addr); 
    server.setTcpHandler(&handler); 

    server.listen();
    loop.loop();

    return 0;
}