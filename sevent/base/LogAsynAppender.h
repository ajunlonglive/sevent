#ifndef SEVENT_BASE_LOGASYNAPPENDER_H
#define SEVENT_BASE_LOGASYNAPPENDER_H

#include "sevent/base/Logger.h"
#include "CountDownLatch.h"
#include "FixedBuffer.h"
#include <stdio.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>


namespace sevent{
class LogFile;

//日志输出(异步输出)
class LogAsynAppender : public LogAppender, noncopyable {
public:
    LogAsynAppender(const std::string &basename = "logfile", int interval = 3,
                    int limit = 25);
    virtual void append(const char *log, int len) override;
    virtual void flush() override;
    virtual void init() override { start(); }
    void process();
    void start();
    void stop();
    void setInterval(int interval);
    void setLimit(int limit);
    void setLogFile(std::unique_ptr<LogFile> &file);
    virtual ~LogAsynAppender();

private:
    static const int largeBufferSize = 4000 * 1000; // 4MB
    std::chrono::seconds flushInterval; //若curBuffer未满,默认每间隔3秒,从缓冲区获取数据并写入
    int buffersLimit; //限制写入数据量(largeBufferSize*limit)

    std::thread thd;
    std::atomic<bool> running;
    std::mutex mtx;
    std::condition_variable cond;
    std::unique_ptr<LogFile> output;
    CountDownLatch latch;

    using Buffer = FixedBuffer<largeBufferSize>;
    std::unique_ptr<Buffer> curBuffer;
    std::unique_ptr<Buffer> nextBuffer;
    std::vector<std::unique_ptr<Buffer>> buffersHolder;
};
//输出到文件(默认500MB或每天0点后滚动文件(每write,check次,检查一次时间));
// basename="./logfile",文件名="[./logfile].20220330-163914.hostname.pid.log"
class LogFile : noncopyable {
public:
    LogFile(const std::string &basename, bool threadSafe = false,
            off_t rollSize = 500 * 1000 * 1000, int check = 1024);
    void write(const char *log, int len);
    void writeInternal(const char *log, int len);
    void flush();
    void rollFile();
    virtual std::string getLogFileName(time_t now); // 生成文件的名字
    virtual ~LogFile();

private:
    const std::string basename;
    bool threadSafe;
    off_t rollSize;
    int check;

    std::mutex mtx;
    int count;
    off_t writtenBytes;
    time_t lastRoll;
    time_t today;
    FILE *fp;
    char fpBuffer[64 * 1024];

    static const int daySecondUnit = 24 * 3600;
};


} // namespace sevent

#endif