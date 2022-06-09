# sevent 一个简单易用的高性能C++网络库

## 简介

sevent是基于**C++17**开发的异步非阻塞网络库, 采用**one loop per thread**模型. 

- 支持windows/linux跨平台
- 支持IPV4, IPV6, http, ssl(https)(可选, 需要openssl)等
- 不依赖第三方库

> seven是看完陈硕的《Linux多线程服务端编程》后萌生的想法. 将muduo改成了支持windows, 并且使用虚函数以面向对象的方式使用(而不是muduo中std::function), 同时移除boost(使用C++17中的std::any等), 还有其他改动参考源码. 目的就是为了简单易用, sevent的s就是simple, 简单的事件驱动库.

## 编译构建

项目采用cmake管理.

- 在linux下, 运行 `start.sh`.  (linux kernel >= 2.6.28)
- 在windows下, 运行`start.bat`, cmake的选项是`-G "MinGW Makefiles"` (也支持MSVC)

查看构建脚本或CMakeLists.txt, 可自定义编译选项; 例如-DENABLE_OPENSSL=ON`, 决定启用openssl

## 示例

```c++
class EchoHandler : public TcpHandler {
    void onMessage(const TcpConnection::ptr &conn, Buffer *buf) {
        string msg = buf->readAllAsString();
        LOG_INFO << "echo " << msg.size() << " bytes";
        conn->send(msg);
    }
};
int main(){
    // 完整示例在sample/
    EventLoop loop;
    TcpServer server(&loop, 12345);    
    EchoHandler handler;
    server.setTcpHandler(&handler); 
    server.listen();
    loop.loop();
    return 0;
}
```

>  更多示例参考**examples和tests文件夹**(重新实现了书中第6章的示例, 文件传输, 空闲连接, socks4a等)



## 类介绍

### *Channel*

&emsp;&emsp;Channel是文件描述符(sockfd)的抽象, 可以通过它来注册和响应IO事件. 通过`(enableRead/WriteEvent)`方法注册事件; 不同的事件类型响应不同的**virtual**方法:`(handleClose/Error/Read/Write)`. Poller直接管理Channel, 每个Channel只属于一个Poller.

### *Poller*

&emsp;&emsp;Poller是select/poll/epoll的抽象. 用于管理Channels(更新/移除事件列表), 每次调用都返回有活跃事件的Channel列表. 每个Poller只属于一个EventLoop. 

> **linux默认使用epoll, windows则使用[wepoll](https://github.com/piscisaureus/wepoll).**

---

### *EventLoop*

&emsp;&emsp;EventLoop就是事件循环, 用于响应IO和定时器事件. 每个线程只能创建一个EventLoop(每个loop独属一个线程), 结合上面, 也就是说一个sockfd只能由一个线程读写. 

```c++
// EventLoop负责的事情
while(1) { 
    channels = poller.poll(); 
    channels.handleEvent(); 
}
```



### *TcpServer*

&emsp;&emsp;TcpServer持有Acceptor类, 而Acceptor是一个Channel, 其创建持有serverFd, 进行bind/listen/accept.

### *TcpClient*

&emsp;&emsp;TcpClient持有Connector类, 而Connector是一个Channel, 其创建持有clientFd, 进行connect.

### *TcpConnection*

&emsp;&emsp;TcpConnection是一个Channel, 持有建立连接后的fd. 声明周期由**shared_ptr**管理, 析构会close(fd). 其send/shutdown/forceClose等方法是**线程安全**的, 非线程安全的方法都标注在头文件.

&emsp;TcpConnection会把接收到的数据和未发送的数据缓存到Buffer. 通过inputBuffer确保收到消息的完整性(比如说在codec中分包, 收到完整消息后才通知程序); 通过outputBuffer确保数据发送完全(注册POLLOUT事件, socket可写就发送缓存中的数据, 所以用户**不必关心数据的发送**).

### *TcpHandler*

&emsp;&emsp;TcpHandler作用于TcpConnection回调. 当事件发生时, 其**virtual**方法会被TcpConnection调用,(`onConnection/onMessage/onClose/onWriteComplete/onHighWaterMark`). 提供给用户进行业务处理.

&emsp;&emsp;**注意TcpHandler应该是长生命周期对象(比TcpConnection长); 若TcpHandler是短生命周期, 应该在其析构前把对应TcpConnection的handler置空(或者保存到TcpConnection的context(小心循环引用)等方法延长生命周期). 因为前者作为指针被调用, 后者则由shared_ptr管理(生命周期可能长于handler, 当事件发生时, 访问野指针).**

---

### *TcpPipeline*

&emsp;&emsp;TcpPipeline是一个特殊的TcpHandler, 类似与netty的pipeline, 通过责任链模式处理传递**std::any &msg**, 用户通过重写**PipelineHandler**, 来自定义业务数据.

>  更多信息请阅读源码, 源码有足够的注释

## Codec

&emsp;&emsp;codec是网络数据与业务数据的间接层, 对数据进行encode/decode. 比如说分包, ssl加解密. sevent可以通过TcpPipeline::addLast(PipelineHandler*)方法进行一个或多个Codec的添加. 

> 可以参考examples/https/中的示例



## 常见问题

### 怎么自定义日志消息格式?

&emsp;&emsp;继承重写LogEventProcessor的虚函数, 并设置到Logger.

### 怎么输出到日志文件?

&emsp;&emsp;默认输出到stdout, 使用LogAsynAppender, 并设置到Logger

### 怎么使用sevent库?

&emsp;&emsp;设置头文件和库文件路径(默认安装到include和lib文件夹), 并链接(-lsevent_net -lsevent_base).(若是windows则还需ws2_32)

### TcpHandler的相关调用?

- 什么时候会调用onWriteComplete?

&emsp;&emsp;发送缓冲区被清空就会调用(低水位回调)

&emsp;&emsp;1. 在一次send(data,len)中, 若一次发送完全, 则会调用

&emsp;&emsp;2. 若发送不完全, 则注册写事件, 处理写事件, 当output Buffer发送完全(可读字节数为0), 则会调用

- 什么时候会调用onClose?
  

&emsp;&emsp; 当发生TcpConnection::handleClose的时候(1.forceClose; 2.read = 0; 3.EPOLLHUP; 4.write = -1;), handleClose会把channel移出监听队列.

- 什么时候调用onHighWaterMark?

&emsp;&emsp; 当发送缓冲区数据大于等于高水位(默认:64MB), 上升边沿触发一次

### forceClose与shutdown的区别?

&emsp;&emsp; forceClose会调用close(fd), 可能会丢失Buffer中的数据; shutdown保证Buffer的数据发送完全, 并且在发送完全后会shutdown(SHUT_WR)

### 怎么进行DNS解析?

&emsp;&emsp; InetAddress::resolve提供阻塞式的解析, 具体看sevent/net/tests/InetAddress_test.cpp

