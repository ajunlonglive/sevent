#ifndef SEVENT_BASE_LOGGER_H
#define SEVENT_BASE_LOGGER_H

#include "CountDownLatch.h"
#include "FixedBuffer.h"
#include "LogStream.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sevent {

class LogEvent;
class LogEventProcessor;
class LogAppender;
class LogFile;
void setUTCOffset(time_t gmtOffsetSecond);

//日志
class Logger : noncopyable {
public:
    enum Level { DEBUG, INFO, WARN, ERROR, FATAL, LEVEL_SIZE };
    static Logger &instance(); //单例
    void append(const char *data, int len);
    void flush();
    void msgBefore(LogEvent &logEv);
    void msgAfter(LogEvent &logEv);

    void log(const char *file, int line, Logger::Level level,
             const std::string &msg);

    // TODO 通过环境变量初始化logLevel getenv("MUDUO_LOG_TRACE")
    void setLogLevel(Level level) { this->level = level; }
    Level getLogLevel() { return this->level; }
    void setLogEventProcessor(std::unique_ptr<LogEventProcessor> &processor);
    void setLogAppender(std::unique_ptr<LogAppender> &output);
    // TEST
    std::unique_ptr<LogAppender> &getLogAppender() { return output; }
    ~Logger() {}

private:
    using Processor_ptr = std::unique_ptr<LogEventProcessor>;
    using Output_ptr = std::unique_ptr<LogAppender>;
    Logger(Level level, Processor_ptr &&processor, Output_ptr &&output);

private:
    Level level;
    std::unique_ptr<LogEventProcessor> processor;
    std::unique_ptr<LogAppender> output;
};

//日志消息加工
class LogEventProcessor : noncopyable {
public:
    //用于自定义消息格式
    virtual void beforeMsgToStream(LogEvent &logEv) = 0;
    virtual void afterMsgToStream(LogEvent &logEv) = 0;
    virtual ~LogEventProcessor() {}
};

//默认的日志消息加工
//默认格式:日期 时间 微秒 线程 级别 正文 - 源文件:行号
// 20220330 16:39:14.818361 72791 DEBUG Hello - test.cpp:15
class DefaultLogProcessor : public LogEventProcessor {
public:
    virtual void beforeMsgToStream(LogEvent &logEv) override;
    virtual void afterMsgToStream(LogEvent &logEv) override;
    static void setShowMicroSecond(bool show);
    const char *formatTime(Timestamp time);
};

//日志消息实体
class LogEvent {
public:
    LogEvent(const char *file, int line, Logger::Level level);
    ~LogEvent(); //析构时输出log到Appender

    Timestamp &getTimestamp() { return this->time; }
    const std::string &getThreadId() { return this->threadId; }
    Logger::Level getLevel() { return this->level; }
    const char *getFile() { return this->file; }
    int getLine() { return this->line; }
    LogStream &getStream() { return this->stream; }

private:
    static std::string inittid();

private:
    const char *file; //源文件名      __FILE__
    int line;         //源文件行号    __LINE__
    // TODO:__FUNC__
    Logger::Level level;
    Timestamp time;
    thread_local static std::string threadId;
    thread_local static LogStream stream; //用于存放log字符串
};

//日志输出(默认同步输出到屏幕,异步文件输出:LogAsynAppender)
class LogAppender {
public:
    virtual void append(const char *log, int len);
    virtual void flush();
    virtual void init() {}
    virtual ~LogAppender() {}
};

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
    ~LogAsynAppender();

private:
    static const int largeBufferSize = 4000 * 1000; // 4MB
    std::chrono::seconds
        flushInterval; //若curBuffer未满,默认每间隔3秒,从缓冲区获取数据并写入
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
//输出到文件(默认500MB滚动文件;)
// basename="./logfile",文件名="[./logfile].20220330-163914.hostname.pid.log"
class LogFile : noncopyable {
public:
    LogFile(const std::string &basename, bool threadSafe = false,
            off_t rollSize = 500 * 1000 * 1000, int check = 1024);
    void write(const char *log, int len);
    void writeInternal(const char *log, int len);
    void flush();
    void rollFile();
    std::string getLogFileName(time_t now);
    ~LogFile();

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
#define LOG_DEBUG                                                              \
    if (sevent::Logger::instance().getLogLevel() <= sevent::Logger::DEBUG)     \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::DEBUG).getStream()
#define LOG_INFO                                                               \
    if (sevent::Logger::instance().getLogLevel() <= sevent::Logger::INFO)      \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::INFO).getStream()
#define LOG_WARN                                                               \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::WARN).getStream()
#define LOG_ERROR                                                              \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::ERROR).getStream()
#define LOG_FATAL                                                              \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::FATAL).getStream()
} // namespace sevent

#endif