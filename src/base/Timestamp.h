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
    static Timestamp now();
    std::string toString();

    static double timeDifference(Timestamp high, Timestamp low);

public:
    static const int microSecondUnit = 1000000;
private:
    int64_t microSecond;
};

} // namespace sevent

#endif