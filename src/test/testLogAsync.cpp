#include "../base/Logger.h"
#include "../base/LogAsynAppender.h"
#include <iostream>
#include <memory>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
using namespace std;
using namespace sevent;

int g_total;
FILE *g_file;

// double timeDifference(Timestamp high, Timestamp low)
// {
//   int64_t diff = high.getMicroSecond() - low.getMicroSecond();
//   return static_cast<double>(diff) / Timestamp::microSecondUnit;
// }

void bench(bool longLog)
{
  int cnt = 0;
  const int kBatch = 1000;
  string empty = " ";
  string longStr(3000, 'X');
  longStr += " ";

  for (int t = 0; t < 30; ++t)
  {
    Timestamp start = Timestamp::now();
    for (int i = 0; i < kBatch; ++i)
    {
      LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
               << (longLog ? longStr : empty)
               << cnt;
      ++cnt;
    }
    Timestamp end = Timestamp::now();
    printf("%f\n", Timestamp::timeDifference(end, start)*1000000/kBatch);
    struct timespec ts = { 0, 500*1000*1000 };
    nanosleep(&ts, NULL);
  }
}

int main() {
    {
      // set max virtual memory to 2GB.
      size_t kOneGB = 1000*1024*1024;
      rlimit rl = { 2*kOneGB, 2*kOneGB };
      setrlimit(RLIMIT_AS, &rl);
    }
    DefaultLogProcessor::setShowMicroSecond(true);
    setUTCOffset(8 * 3600);

    unique_ptr<LogAppender> p(new LogAsynAppender("./logtest"));
    Logger &logger = Logger::instance();
    logger.setLogAppender(p);

    printf("pid = %d\n", getpid());

    bench(false);

    return 0;
}