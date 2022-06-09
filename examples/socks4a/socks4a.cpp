#include "sevent/base/Logger.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/EndianOps.h"
#include "sevent/net/TcpClient.h"
#include "sevent/net/TcpHandler.h"
#include "sevent/net/TcpServer.h"
#include <iostream>
#include <string.h>
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
    Tunnel(EventLoop *loop, const InetAddress &addr,
           const TcpConnection::ptr &serConn)
        : client(loop, addr), serverConn(serConn) {
        LOG_INFO << "Tunnel " << serverConn->getPeerAddr().toStringIpPort()
                 << " <-> " << addr.toStringIpPort() << " client = " << &client;
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
    void shutdown() {
        LOG_INFO << "Tunnel shutdown client = " << &client;
        client.shutdown(); 
    }

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
        LOG_TRACE << "Tunnel as client recv " << buf->readableBytes() << " bytes";
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
        // clientConn.reset(); // 由Tunnel析构函数释放也行
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

class Socks4aHandler : public TcpHandler {
public:
    Socks4aHandler(EventLoop *loop) : loop(loop){}

private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << "Tunnel as server onConnection";
        conn->setTcpNoDelay(true);
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        LOG_TRACE << "Tunnel as server recv " << buf->readableBytes() << " bytes";
        // 解析socks4a报文
        // 版本一:(服务器ip+port)
        // ver(1字节) cmd(1字节) port(2字节) ip(4字节)
        // 版本二:(服务器域名+port)
        // ver(1字节) cmd(1字节) port(2字节) ip(4字节,无效) 用户id字符串('\0'终止) 域名(同上)
        if (tunnels.find(conn->getId()) == tunnels.end()) {
            if (buf->readableBytes() > 128) {
                conn->shutdown();
            } else if (buf->readableBytes() > 8) {
                const char *begin = buf->peek() + 8;
                const char *end = buf->peek() + buf->readableBytes();
                const char *where = std::find(begin, end, '\0');
                if (where != end) {
                    char ver = buf->peek()[0];
                    char cmd = buf->peek()[1];
                    const void *port = buf->peek() + 2;
                    const void *ip = buf->peek() + 4;

                    sockaddr_in addr;
                    memset(&addr, 0, sizeof addr);
                    addr.sin_family = AF_INET;
                    addr.sin_port = *static_cast<const uint16_t *>(port);
                    addr.sin_addr.s_addr = *static_cast<const uint32_t *>(ip);
                    // 根据ip判断版本一,二 (无效ip前3字节为0, 最后1字节不为0)
                    bool socks4a = sockets::netToHost32(addr.sin_addr.s_addr) < 256;
                    bool okay = false;
                    if (socks4a) {
                        const char *endOfHostName = std::find(where + 1, end, '\0');
                        if (endOfHostName != end) {
                            string hostname = where + 1;
                            where = endOfHostName;
                            LOG_INFO << "Socks4a host name " << hostname;
                            InetAddress tmp;
                            // 解析域名(阻塞)
                            if (InetAddress::resolve(hostname, &tmp)) {
                                addr.sin_addr.s_addr = tmp.getIpNet();
                                okay = true;
                            }
                        } else {
                            return;
                        }
                    } else {
                        okay = true;
                    }

                    InetAddress serverAddr(addr);
                    // 4, 1代表首次发送
                    if (ver == 4 && cmd == 1 && okay) {
                        shared_ptr<Tunnel> tunnel(new Tunnel(loop, serverAddr, conn));
                        tunnel->connect();
                        tunnels[conn->getId()] = tunnel;
                        buf->retrieveUntil(where + 1);
                        char response[] = "\000\x5aUVWXYZ"; // 0x5A:批准请求
                        memcpy(response + 2, &addr.sin_port, 2);
                        memcpy(response + 4, &addr.sin_addr.s_addr, 4);
                        conn->send(response, 8);
                    } else {
                        char response[] = "\000\x5bUVWXYZ";// 0x5B:拒绝请求
                        conn->send(response, 8);
                        conn->shutdown();
                    }
                }
            }
        } else if (!conn->getContext().empty()) {
            const TcpConnectionPtr &clientConn = getClientConntion(conn);
            if (clientConn)
                clientConn->send(buf);
        }
    }
    void onClose(const TcpConnection::ptr &conn) {
        // 当客户端断开连接(注意有可能未连接服务端就断开连接)
        LOG_INFO << "Tunnel as server onClose";
        TunnelsMap::iterator it = tunnels.find(conn->getId());
        if (it != tunnels.end()) {
            it->second->shutdown(); // 关闭服务端连接
            tunnels.erase(it);
        }
    }
    void onWriteComplete(const TcpConnection::ptr &conn) {
        const TcpConnection::ptr &clientConn = getClientConntion(conn);
        if (clientConn) {
            if (!clientConn->isReading()) {
                LOG_INFO
                    << "Tunnel as server onWriteComplete, enable server read";
                clientConn->enableRead();
            }
        }
    }
    void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {
        // 从服务端收到数据, 转发给客户端到达高水位(所以应该停止读服务端)
        LOG_INFO << "Tunnel as server onHighWaterMark, disable server read, "
                    "curHight = " << curHight;
        const TcpConnection::ptr &clientConn = getClientConntion(conn);
        if (clientConn)
            clientConn->disableRead();
    }
    const TcpConnection::ptr & getClientConntion(const TcpConnection::ptr &conn) {
        std::any &a = conn->getContext("client");
        if (a.has_value()) {
            TcpConnection::ptr &clientConn = std::any_cast<TcpConnection::ptr &>(a);
            return clientConn;
        }
        return g_nullConnection;
    }

private:
    using TunnelsMap = unordered_map<int64_t, shared_ptr<Tunnel>>;
    EventLoop *loop;
    unordered_map<int64_t, shared_ptr<Tunnel>> tunnels;
};

// 客户端 <---> tunnel <---> 服务端
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    EventLoop loop;
    TcpServer server(&loop, 12346);
    Socks4aHandler handler(&loop);
    server.setTcpHandler(&handler);

    server.listen();
    loop.loop();

    return 0;
}