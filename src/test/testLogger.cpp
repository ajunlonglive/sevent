#include <iostream>
#include "../base/Logger.h"
#include <memory>
#include <string>
#include <stdio.h>
using namespace std;
using namespace sevent;

int g_total;
FILE *g_file;


void bench(const char* type)
{
    Timestamp start = Timestamp::now();
    g_total = 0;

    int n = 1000 * 1000;
    // int n = 5;
    const bool kLongLog = false;
    string empty = " ";
    string longStr(3000, 'X');
    longStr += " ";

    DefaultLogProcessor::setShowMicroSecond(true);

    for (int i = 0; i < n; ++i) {
      LOG_INFO << "Hello 0123456789"
               << " abcdefghijklmnopqrstuvwxyz" << (kLongLog ? longStr : empty)
               << i;
    }
  Timestamp end = Timestamp::now();
  double seconds = Timestamp::timeDifference(end, start);
  printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
         type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

class MyAppender : public LogAppender {
public:
    void append(const char *log, int len){
        // cout<<log;
        g_total += len;
        fwrite(log, 1, len, g_file);
    }
    void flush(){
        fflush(g_file);
    }
  
};

int main() {
    DefaultLogProcessor::setShowMicroSecond(true);
    // SEVENT::setUTCOffset(8 * 3600);
    unique_ptr<LogAppender> p(new MyAppender);
    Logger &logger = Logger::instance();
    logger.setLogAppender(p);

    char buffer[64*1024];
    g_file = fopen("/dev/null", "w");
    // g_file = fopen("./log.txt", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/dev/null");
    fclose(g_file);

    return 0;
}