#ifndef SEVENT_NET_TCPHANDLER_H
#define SEVENT_NET_TCPHANDLER_H

#include "TcpConnection.h"
namespace sevent {
namespace net {
class TcpHandler {
public:
    // for override
    virtual void onConnection(const TcpConnection::ptr &conn) {}
    virtual void onMessage(const TcpConnection::ptr &conn, Buffer &buf) {}
    virtual void onClose(const TcpConnection::ptr &conn) {}
    virtual void onWriteComplete(const TcpConnection::ptr &conn) {} 
    virtual void onHighWaterMark(const TcpConnection::ptr &conn, size_t curHight) {}
    // TODO
    // 什么时候会调用onWriteComplete?
        // 发送缓冲区被清空就会调用(低水位回调)
        // 1.在一次send(data,len)中, 若一次发送完全(write字节数 = len), 则会调用
        // 2.若发送不完全, 注册写事件, handleWrite处理写事件, 当outputBuf的可读字节数 = 0(发送完全), 则会调用
    // 什么时候会调用onClose?
        // 当发生TcpConnection::handleClose的时候(1.forceClose; 2.read = 0; 3.EPOLLHUP)
    // 什么时候调用onHighWaterMark?
	    // 要发送的数据+发送缓冲区当前数据超过指定大小(并且当前发送缓冲区数据小于该值),就会触发
};

} // namespace net
} // namespace sevent

#endif