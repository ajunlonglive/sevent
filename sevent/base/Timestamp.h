#ifndef SEVENT_BASE_TIMESTAMP_H
#define SEVENT_BASE_TIMESTAMP_H

#include <stdint.h>
#include <string>

namespace sevent{

class Timestamp {
public:
    Timestamp():microSecond(0){}
    explicit Timestamp(int64_t microSecond):microSecond(microSecond){}
    int64_t getMicroSecond() { return microSecond; }
    time_t getSecond() { return static_cast<time_t>(microSecond / microSecondUnit); }
    // microseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
    static Timestamp now();
    std::string toString();

    bool operator<(Timestamp other) { return microSecond < other.microSecond; }
    bool operator==(Timestamp other) { return microSecond == other.microSecond; }
    bool operator<=(Timestamp other) { return microSecond <= other.microSecond; }

    static double timeDifference(Timestamp high, Timestamp low);
    static Timestamp addTime(Timestamp t, int64_t millisecond);
    // static Timestamp addSecond(Timestamp t, int64_t second);
    // static Timestamp addMinute(Timestamp t, int64_t minute);
    // static Timestamp addHour(Timestamp t, int64_t hour);
    // static Timestamp addDay(Timestamp t, int64_t day);
    // static Timestamp addMonth(Timestamp t, int64_t Month);

public:
    static const int microSecondUnit = 1000000; // second <-> micorsecond
private:
    int64_t microSecond;
};

} // namespace sevent

#endif