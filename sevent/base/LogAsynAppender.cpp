#include "sevent/base/LogAsynAppender.h"
#include "sevent/base/CommonUtil.h"
#include <time.h>

using namespace sevent;
using namespace std;

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
                     "Dropped log messages at %s, %d larger buffers\n",
                     Timestamp::now().toString().c_str(),
                     static_cast<int>(buffers_bak.size() - 2));
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
    size_t n = CommonUtil::fwrite_unlocked(log, 1, len, fp);
    size_t remain = len - n;
    while (remain > 0) {
        size_t tmp = CommonUtil::fwrite_unlocked(log + n, 1, remain, fp);
        if (tmp == 0) {
            int err = ferror(fp);
            if (err) {
                // thread_local char errbuf[512] = {0};
                fprintf(stderr, "LogFile::writeInternal() failed %s\n",
                        CommonUtil::strerror_tl(err));
            }
            break;
        }
        remain -= tmp;
    }
    writtenBytes += len;
    // 默认500MB或每天0点后滚动文件(每write(),check次,检查一次);
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
        setvbuf(fp, fpBuffer, _IOFBF, sizeof(fpBuffer));
    }
}
string LogFile::getLogFileName(time_t now) {
    string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char buf[32];
    struct tm tm_time;
    now += g_gmtOffsetSec;
    CommonUtil::gmtime_r(&now, &tm_time);
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
