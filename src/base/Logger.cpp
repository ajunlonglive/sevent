#include "Logger.h"
#include "CurrentThread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

using namespace sevent;
using namespace std;

//各线程缓存tid字符串
thread_local std::string LogEvent::threadId = LogEvent::inittid();
thread_local LogStream LogEvent::stream;

namespace sevent {

const char *LogLevelName[Logger::LEVEL_SIZE] = {
    "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};
time_t g_gmtOffsetSec = 0;
bool g_showMicroSecond = false;

void setUTCOffset(time_t gmtOffsetSecond) {
    sevent::g_gmtOffsetSec = gmtOffsetSecond;
}
} // namespace sevent

/********************************************************************
 *                              Logger
 * ******************************************************************/

Logger::Logger(Level level, Processor_ptr &&processor, Output_ptr &&output)
    : level(level), processor(std::move(processor)), output(std::move(output)) {
    this->output->init();
}
//构造默认实例
Logger &Logger::instance() {
    static Logger logger(Level::INFO,
                         unique_ptr<LogEventProcessor>(new DefaultLogProcessor),
                         unique_ptr<LogAppender>(new LogAppender));
    return logger;
}
void Logger::log(const char *file, int line, Level level, const string &msg) {
    LogEvent logEv(file, line, level);
    logEv.getStream() << msg;
}

void Logger::append(const char *data, int len) { output->append(data, len); }
void Logger::flush() { output->flush(); }
void Logger::msgBefore(LogEvent &logEv) { processor->beforeMsgToStream(logEv); }
void Logger::msgAfter(LogEvent &logEv) { processor->afterMsgToStream(logEv); }

void Logger::setLogEventProcessor(unique_ptr<LogEventProcessor> &processor) {
    this->processor = std::move(processor);
}
void Logger::setLogAppender(unique_ptr<LogAppender> &output) {
    this->output = std::move(output);
    this->output->init();
}

/********************************************************************
 *                          LogEventProcessor
 * ******************************************************************/

/********************************************************************
 *                          DefaultLogProcessor
 * ******************************************************************/
void DefaultLogProcessor::beforeMsgToStream(LogEvent &logEv) {
    //日期 时间 微秒 线程 级别 正文 - 源文件:行号
    LogStream &stream = logEv.getStream();
    //时间固定长度为17 或 24(微秒部分)
    stream << StreamItem(formatTime(logEv.getTimestamp()),
                         g_showMicroSecond ? 24 : 17);
    stream << ' ';
    //线程,后有空格" "
    stream << logEv.getThreadId();
    //loglevel固定长度为6
    stream << StreamItem(LogLevelName[logEv.getLevel()], 6);
    //errno
    int errNumber = logEv.getErrno();
    if (errNumber) {
        thread_local char errnoBuf[512];
        stream << strerror_r(errNumber, errnoBuf, sizeof(errnoBuf));
        stream << "(errno=" << errNumber << ") ";
    }
}
void DefaultLogProcessor::afterMsgToStream(LogEvent &logEv) {
    // - 源文件:行号
    LogStream &stream = logEv.getStream();
    stream << StreamItem(" - ", 3);
    stream << logEv.getFile();
    stream << ':';
    stream << logEv.getLine();
    stream << '\n';
}

const char *DefaultLogProcessor::formatTime(Timestamp time) {
    //时间和时间字符串缓存
    thread_local time_t lastSecond = 0;
    thread_local char timeStrCache[64] = {0};
    int64_t microSceond = time.getMicroSecond();
    time_t second =
        static_cast<time_t>(microSceond / Timestamp::microSecondUnit);
    int microsec = static_cast<int>(microSceond % Timestamp::microSecondUnit);
    //若不同时间,全部重新格式化;若同一秒内,只格式化微秒部分
    if (second != lastSecond) {
        lastSecond = second;
        struct tm tm_time;
        //默认(UTC+0);
        second += g_gmtOffsetSec;
        gmtime_r(&second, &tm_time);
        //固定长度 17,实测strftime比snprintf快
        strftime(timeStrCache, sizeof(timeStrCache), "%Y%m%d %H:%M:%S",
                 &tm_time);
    }

    if (g_showMicroSecond) {
        //固定长度7
        snprintf(timeStrCache + 17, 9, ".%06d", microsec);
    }
    return timeStrCache;
}

void DefaultLogProcessor::setShowMicroSecond(bool show) {
    sevent::g_showMicroSecond = show;
}

/********************************************************************
 *                          LogEvent
 * ******************************************************************/
LogEvent::LogEvent(const char *file, int line, Logger::Level level,int err)
    : file(file), line(line), level(level),errNum(err), time(Timestamp::now()) {
    //参考muduo 5.2章:GCC的strrchr()对于字符串字面量可以在编译期求值
    const char *slash = strrchr(file, '/');
    if (slash)
        this->file = slash + 1;
    Logger::instance().msgBefore(*this);
}
LogEvent::~LogEvent() {
    Logger::instance().msgAfter(*this);
    LogStream::Buffer &buf = this->stream.getBuffer();
    Logger::instance().append(buf.begin(), buf.length());
    if (level == Logger::FATAL) {
        Logger::instance().flush();
        abort();
    }
    this->stream.reset();
}

std::string LogEvent::inittid() {
    //缓存每个线程的tid字符串
    char buf[32] = {0};
    if (threadId.empty()) {
        snprintf(buf, sizeof(buf), "%5d ", CurrentThread::gettid());
    }
    return buf;
}

/********************************************************************
 *                          LogAppender
 * ******************************************************************/
void LogAppender::append(const char *log, int len) {
    fwrite(log, 1, len, stdout);
}
void LogAppender::flush() { fflush(stdout); }
