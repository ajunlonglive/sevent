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

Timestamp Timestamp::addTime(Timestamp t, int64_t millisecond) {
    int64_t msec = static_cast<int64_t>(millisecond * 1000);
    return Timestamp(t.getMicroSecond() + msec);
}
// Timestamp Timestamp::addTime(Timestamp t, double second) {
//     int64_t msec = static_cast<int64_t>(second * Timestamp::microSecondUnit);
//     return Timestamp(t.getMicroSecond() + msec);
// }

// Timestamp Timestamp::addSecond(Timestamp t, int64_t second) {
//     return addTime(t, second * 1000);
// }
// Timestamp Timestamp::addMinute(Timestamp t, int64_t minute) {
// }
// Timestamp Timestamp::addHour(Timestamp t, int64_t hour) {
// }
// Timestamp Timestamp::addDay(Timestamp t, int64_t day) {
// }