#include "Timestamp.h"
#include <sys/time.h>

using namespace sevent;
using namespace std;

Timestamp Timestamp::now(){
    timeval tv;
    gettimeofday(&tv, nullptr);
    return Timestamp(tv.tv_sec * 1000000ul + tv.tv_usec);    
}

string Timestamp::toString() {
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecond / microSecondUnit);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", &tm_time);
    return buf;
}

double Timestamp::timeDifference(Timestamp high, Timestamp low){
    int64_t diff = high.getMicroSecond() - low.getMicroSecond();
    return static_cast<double>(diff) / Timestamp::microSecondUnit;
}