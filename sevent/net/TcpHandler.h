#ifndef SEVENT_NET_TCPHANDLER_H
#define SEVENT_NET_TCPHANDLER_H

#include "sevent/net/TcpConnection.h"
namespace sevent {
using TcpConnectionPtr = sevent::net::TcpConnection::ptr;
namespace net {
class TcpHandler : noncopyable {
public:
    // for override
    virtual void onConnection(const TcpConnection::ptr &conn) {}
    virtual void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {}
    virtual void onClose(const TcpConnection::ptr &conn) {}
    virtual void onWriteComplete(const TcpConnection::ptr &conn) {}
    virtual void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {}
    virtual ~TcpHandler() = default;
    // TODO
    // 什么时候会调用onWriteComplete?
        // 发送缓冲区被清空就会调用(低水位回调)
        // 1.在一次send(data,len)中, 若一次发送完全(write return = len), 则会调用
        // 2.若发送不完全, 则注册写事件, 处理写事件, 当output Buffer发送完全(可读字节数为0), 则会调用
    // 什么时候会调用onClose?
        // 当发生TcpConnection::handleClose的时候
        // (1.forceClose; 2.read = 0; 3.EPOLLHUP; 4.write = -1; 5.TcpClient.shutdown)
    // 什么时候调用onHighWaterMark?
	    // 当发送缓冲区数据(output Buffer)大于等于高水位(默认:64MB)
        // 上升边沿触发一次(发送缓冲区从低水位到高水位)
    //所有的方法, 都是在conn所属的ownerLoop中被调用
};

} // namespace net
} // namespace sevent

#endif