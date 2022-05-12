#ifndef SEVENT_BASE_LOGGER_H
#define SEVENT_BASE_LOGGER_H

#include "sevent/base/LogStream.h"
#include "sevent/base/noncopyable.h"
#include "sevent/base/Timestamp.h"
#include <memory>
#include <string>
#include <thread>

namespace sevent {

class LogEvent;
class LogEventProcessor;
class LogAppender;
extern time_t g_gmtOffsetSec;
//默认UTC+0,只影响格式化时间;例如 中国标准时间=UTC+8 setUTCOffset(8*3600)
void setUTCOffset(time_t gmtOffsetSecond);
// const char *strerror_tl(int err);

//日志
class Logger : noncopyable {
public:
    enum Level { TRACE, DEBUG, INFO, WARN, ERROR_, FATAL, LEVEL_SIZE };
    static Logger &instance(); //单例
    void append(const char *data, int len);
    void flush();
    void msgBefore(LogEvent &logEv);
    void msgAfter(LogEvent &logEv);
    void log(const char *file, int line, Logger::Level level,
             const std::string &msg);
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
    // 通过环境变量初始化SEVENT_LOG_TRACE:level 参考muduo:getenv("MUDUO_LOG_TRACE")
    static Level initLevel();

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
    LogEvent(const char *file, int line, Logger::Level level,bool isErr = false);
    ~LogEvent(); //析构时输出log到Appender

    Timestamp &getTimestamp() { return this->time; }
    const std::string &getThreadId() { return this->threadId; }
    Logger::Level getLevel() { return this->level; }
    const char *getFile() { return this->file; }
    int getLine() { return this->line; }
    int getErrno() { return this->errNum; }
    LogStream &getStream() { return this->stream; }

private:
    static std::string inittid();

private:
    // TODO:__func__
    const char *file; //源文件名      __FILE__
    int line;         //源文件行号    __LINE__
    Logger::Level level;
    int errNum;
    Timestamp time;
    const std::string &threadId;
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

#define LOG_TRACE                                                              \
    if (sevent::Logger::instance().getLogLevel() <= sevent::Logger::TRACE)     \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::TRACE).getStream()
#define LOG_DEBUG                                                              \
    if (sevent::Logger::instance().getLogLevel() <= sevent::Logger::DEBUG)     \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::DEBUG).getStream()
#define LOG_INFO                                                               \
    if (sevent::Logger::instance().getLogLevel() <= sevent::Logger::INFO)      \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::INFO).getStream()
#define LOG_WARN                                                               \
    if (sevent::Logger::instance().getLogLevel() <= sevent::Logger::WARN)  \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::WARN).getStream()
#define LOG_ERROR                                                              \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::ERROR_).getStream()
#define LOG_FATAL                                                              \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::FATAL).getStream()
#define LOG_SYSERR                                                             \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::ERROR_,true).getStream()
#define LOG_SYSFATAL                                                             \
    sevent::LogEvent(__FILE__, __LINE__, sevent::Logger::FATAL,true).getStream()
} // namespace sevent

#endif