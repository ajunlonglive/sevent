#include <iostream>
#include <deque>
#include <unordered_set>
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

class EchoHandler : public TcpHandler {
public:
    explicit EchoHandler(EventLoop *loop, int idleTime) { 
        connections.resize(idleTime);
        // 每隔1s, 移除第一个桶
        loop->addTimer(1000, std::bind(&EchoHandler::onTimer, this), 1000);
    }

private:
    void onConnection(const TcpConnection::ptr &conn) {
        LOG_INFO << conn->getPeerAddr().toStringIpPort() << " -> "
                 << conn->getLocalAddr().toStringIpPort();
        Connection::ptr sp(new Connection(conn));
        Connection::wptr wp(sp);
        connections.back().insert(sp);
        conn->setContext("idle", wp);
    }
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "echo " << msg.size() << " bytes, "
                 << "data received at " << conn->getPollTime().toString();
        conn->send(msg);
        Connection::wptr &wp = std::any_cast<Connection::wptr&>(conn->getContext("idle"));
        Connection::ptr sp = wp.lock();
        if (sp) {
            // 可以优化: 保存上次插入位置, 检查是否有变化, 有变化则插入新的删除旧的 
            connections.back().insert(sp);
        }
    }

    void onTimer() {
        LOG_TRACE << "onTimer - remove head, bucket size = " << connections.size();
        connections.pop_front();
        connections.push_back(ConnectionSet());
    }

private:
    // Connection:持有TcpConnection的weak_ptr, (并由shared_ptr管理), 析构时关闭连接
    class Connection {
        public:
            using ptr = shared_ptr<Connection>;
            using wptr = weak_ptr<Connection>;
            explicit Connection(const TcpConnection::ptr &conn) : weakConnection(conn) {}
            ~Connection() {
                TcpConnection::ptr conn = weakConnection.lock();
                if (conn)
                    conn->shutdown();
            }
        private:
            weak_ptr<TcpConnection> weakConnection;
    };
    using ConnectionSet = unordered_set<shared_ptr<Connection>>;
    deque<ConnectionSet> connections; // 持有Connection
};
// 剔除空闲连接, idleTime秒
// 另外一种方法: 
    // 1.用list保存TcpConnection的weak_ptr
    // 2.1 onConnection:TcpConnection保存list的位置和lastRecvTime(setContext)
    // 2.2 onMessage: 更新lastRecvTime和插入到list尾部
    // 3.定时器, 每隔1s检查list前面的TcpConnection的lastRecvTime
int main(int argc, char **argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    int idleTime = 8;
    if (argc > 1)
        idleTime = atoi(argv[1]);
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    EchoHandler handler(&loop, idleTime); 
    server.setTcpHandler(&handler); 

    server.listen();
    loop.loop();

    return 0;
}