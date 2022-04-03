#include "logger.h"
#include "CommonUtil.h"
#include "CurrentThread.h"
#include "Timestamp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
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
//默认UTC+0,只影响格式化时间;例如 中国标准时间=UTC+8 setUTCOffset(8*3600)
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
    // loglevel固定长度为6
    stream << StreamItem(LogLevelName[logEv.getLevel()], 6);
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
LogEvent::LogEvent(const char *file, int line, Logger::Level level)
    : file(file), line(line), level(level), time(Timestamp::now()) {
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
        // abort();
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
/********************************************************************
 *                  LogAsynAppender : LogAppender
 * ******************************************************************/
void LogAsynAppender::append(const char *log, int len) {
    lock_guard<mutex> lock(this->mtx);
    if (curBuffer->remain() > len) {
        curBuffer->append(log, len);
    } else {
        // curBuffer已满,先移进bufferHolders
        buffersHolder.push_back(std::move(curBuffer));
        if (nextBuffer)
            curBuffer = std::move(nextBuffer);
        else
            curBuffer.reset(new Buffer); //因为共4个缓存,很少用到
        //添加log到新的curBuffer,并唤醒写入线程
        curBuffer->append(log, len);
        cond.notify_one();
    }
}
void LogAsynAppender::flush() {
    cond.notify_one();
    output->flush();
}
void LogAsynAppender::process() {
    unique_ptr<Buffer> curbuf_bak(new Buffer);
    unique_ptr<Buffer> nextbuf_bak(new Buffer);
    std::vector<std::unique_ptr<Buffer>> buffers_bak;
    buffers_bak.reserve(16);
    this->latch.countDown();
    //通过unique_ptr管理Buffer;ptr等于nullptr,说明Buffer转移了(正在被使用)
    while (this->running) {
        unique_lock<mutex> uLock(this->mtx);
        if (buffersHolder.empty()) {
            cond.wait_for(uLock, flushInterval);
            if (curBuffer->empty()) {
                uLock.unlock();
                continue;
            }
        }
        buffersHolder.push_back(std::move(curBuffer));
        curBuffer = std::move(curbuf_bak);
        buffers_bak.swap(buffersHolder);
        if (!nextBuffer)
            nextBuffer = std::move(nextbuf_bak);
        uLock.unlock();
        //临界区结束,数据交换到buffers_bak里;开始写入数据

        if (buffers_bak.size() > static_cast<size_t>(buffersLimit)) {
            char buf[256]{0};
            snprintf(buf, sizeof(buf),
                     "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toString().c_str(),
                     buffers_bak.size() - 2);
            fputs(buf, stderr);
            output->write(buf, static_cast<int>(strlen(buf)));
            buffers_bak.resize(2);
        }

        for (auto &buffer : buffers_bak) {
            output->write(buffer->begin(), buffer->size());
        }
        //复原 curbuf_bak,nextbuf_bak
        if (!curbuf_bak) {
            curbuf_bak = std::move(buffers_bak.back());
            curbuf_bak->reset();
            buffers_bak.pop_back();
        }
        if (!nextbuf_bak) {
            nextbuf_bak = std::move(buffers_bak.back());
            nextbuf_bak->reset();
            buffers_bak.pop_back();
        }

        buffers_bak.clear();
        output->flush();
    }
    output->flush();
}
void LogAsynAppender::start() {
    this->running = true;
    this->thd = thread(&LogAsynAppender::process, this);
    this->latch.wait();
}
void LogAsynAppender::stop() {
    this->running = false;
    this->cond.notify_one();
    this->thd.join();
}
LogAsynAppender::LogAsynAppender(const string &basename, int interval,
                                 int limit)
    : flushInterval(interval), buffersLimit(limit < 4 ? 4 : limit),
      running(false), output(new LogFile(basename)), latch(1),
      curBuffer(new Buffer), nextBuffer(new Buffer) {}

LogAsynAppender::~LogAsynAppender() {
    if (this->running)
        stop();
}
void LogAsynAppender::setInterval(int interval) {
    flushInterval = std::chrono::seconds(interval);
}
void LogAsynAppender::setLimit(int limit) {
    buffersLimit = limit < 4 ? 4 : limit;
}
void LogAsynAppender::setLogFile(std::unique_ptr<LogFile> &file) {
    output = std::move(file);
}
/********************************************************************
 *                          LogFile
 * ******************************************************************/
LogFile::LogFile(const string &basename, bool threadSafe, off_t rollSize,
                 int check)
    : basename(basename), threadSafe(threadSafe), rollSize(rollSize),
      check(check), count(0), writtenBytes(0), lastRoll(0), today(0),
      fp(nullptr) {
    rollFile();
}

void LogFile::write(const char *log, int len) {
    if (threadSafe) {
        lock_guard<mutex> lg(this->mtx);
        writeInternal(log, len);
    } else {
        writeInternal(log, len);
    }
}
void LogFile::writeInternal(const char *log, int len) {
    size_t n = fwrite_unlocked(log, 1, len, fp);
    size_t remain = len - n;
    while (remain > 0) {
        size_t tmp = fwrite_unlocked(log + n, 1, remain, fp);
        if (tmp == 0) {
            int err = ferror(fp);
            if (err) {
                thread_local char errbuf[512] = {0};
                fprintf(stderr, "LogFile::writeInternal() failed %s\n",
                        strerror_r(err, errbuf, sizeof(errbuf)));
            }
            break;
        }
        remain -= tmp;
    }
    writtenBytes += len;
    // 默认500MB或每天0点后滚动文件(每check次,检查一次);
    if (writtenBytes > rollSize) {
        rollFile();
    } else {
        ++count;
        if (count >= check) {
            count = 0;
            time_t now = time(NULL);
            time_t curday = now / daySecondUnit * daySecondUnit;
            if (today != curday)
                rollFile();
        }
    }
}

void LogFile::flush() {
    if (threadSafe) {
        lock_guard<mutex> lg(this->mtx);
        fflush(fp);
    } else {
        fflush(fp);
    }
}

void LogFile::rollFile() {
    //构建新的文件名,fp指向新的文件
    time_t now = time(NULL);
    time_t curday = now / daySecondUnit * daySecondUnit;
    string filename = getLogFileName(now);
    if (now <= lastRoll)
        return; //以防打开相同文件
    FILE *tmp = fopen(filename.c_str(), "a");
    if (tmp) {
        if (fp)
            fclose(fp);
        writtenBytes = 0;
        lastRoll = now;
        today = curday;
        fp = tmp;
        setbuffer(fp, fpBuffer, sizeof(fpBuffer));
    }
}
string LogFile::getLogFileName(time_t now) {
    string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char buf[32];
    struct tm tm_time;
    now += g_gmtOffsetSec;
    gmtime_r(&now, &tm_time);
    strftime(buf, sizeof buf, ".%Y%m%d-%H%M%S.", &tm_time);
    filename += buf;
    filename += CommonUtil::getHostname();
    filename += '.';
    filename += to_string(CommonUtil::getPid());
    filename += ".log";
    return filename;
}

LogFile::~LogFile() {
    if (fp)
        fclose(fp);
}
