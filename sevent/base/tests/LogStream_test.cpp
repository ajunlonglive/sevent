#include "myassert.h"
#include "sevent/base/LogStream.h"
#include <iostream>
#include <limits>

using namespace std;
using namespace sevent;

void testLogStreamBooleans() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();
    myassert(buf.toString() == string(""));
    os << true;
    myassert(buf.toString() == string("1"));
    os << '\n';
    myassert(buf.toString() == string("1\n"));
    os << false;
    myassert(buf.toString() == string("1\n0"));
}

void testLogStreamIntegers() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();
    myassert(buf.toString() == string(""));
    os << 1;
    myassert(buf.toString() == string("1"));
    os << 0;
    myassert(buf.toString() == string("10"));
    os << -1;
    myassert(buf.toString() == string("10-1"));
    os.reset();

    os << 0 << " " << 123 << 'x' << 0x64;
    myassert(buf.toString() == string("0 123x100"));
}

void testLogStreamIntegerLimits() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();
    os << -2147483647;
    myassert(buf.toString() == string("-2147483647"));
    os << static_cast<int>(-2147483647 - 1);
    myassert(buf.toString() == string("-2147483647-2147483648"));
    os << ' ';
    os << 2147483647;
    myassert(buf.toString() == string("-2147483647-2147483648 2147483647"));
    os.reset();

    os << std::numeric_limits<int16_t>::min();
    myassert(buf.toString() == string("-32768"));
    os.reset();

    os << std::numeric_limits<int16_t>::max();
    myassert(buf.toString() == string("32767"));
    os.reset();

    os << std::numeric_limits<uint16_t>::min();
    myassert(buf.toString() == string("0"));
    os.reset();

    os << std::numeric_limits<uint16_t>::max();
    myassert(buf.toString() == string("65535"));
    os.reset();

    os << std::numeric_limits<int32_t>::min();
    myassert(buf.toString() == string("-2147483648"));
    os.reset();

    os << std::numeric_limits<int32_t>::max();
    myassert(buf.toString() == string("2147483647"));
    os.reset();

    os << std::numeric_limits<uint32_t>::min();
    myassert(buf.toString() == string("0"));
    os.reset();

    os << std::numeric_limits<uint32_t>::max();
    myassert(buf.toString() == string("4294967295"));
    os.reset();

    os << std::numeric_limits<int64_t>::min();
    myassert(buf.toString() == string("-9223372036854775808"));
    os.reset();

    os << std::numeric_limits<int64_t>::max();
    myassert(buf.toString() == string("9223372036854775807"));
    os.reset();

    os << std::numeric_limits<uint64_t>::min();
    myassert(buf.toString() == string("0"));
    os.reset();

    os << std::numeric_limits<uint64_t>::max();
    myassert(buf.toString() == string("18446744073709551615"));
    os.reset();

    int16_t a = 0;
    int32_t b = 0;
    int64_t c = 0;
    os << a;
    os << b;
    os << c;
    myassert(buf.toString() == string("000"));
}

void testLogStreamFloats() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();

    os << 0.0;
    myassert(buf.toString() == string("0"));
    os.reset();

    os << 1.0;
    myassert(buf.toString() == string("1"));
    os.reset();

    os << 0.1;
    myassert(buf.toString() == string("0.1"));
    os.reset();

    os << 0.05;
    myassert(buf.toString() == string("0.05"));
    os.reset();

    os << 0.15;
    myassert(buf.toString() == string("0.15"));
    os.reset();

    double a = 0.1;
    os << a;
    myassert(buf.toString() == string("0.1"));
    os.reset();

    double b = 0.05;
    os << b;
    myassert(buf.toString() == string("0.05"));
    os.reset();

    double c = 0.15;
    os << c;
    myassert(buf.toString() == string("0.15"));
    os.reset();

    os << a + b;
    myassert(buf.toString() == string("0.15"));
    os.reset();

    os << 1.23456789;
    myassert(buf.toString() == string("1.23456789"));
    os.reset();

    os << 1.234567;
    myassert(buf.toString() == string("1.234567"));
    os.reset();

    os << -123.456;
    myassert(buf.toString() == string("-123.456"));
    os.reset();
}

void testLogStreamVoid() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();

    os << static_cast<void *>(0);
    myassert(buf.toString() == string("0x0"));
    os.reset();

    os << reinterpret_cast<void *>(8888);
    myassert(buf.toString() == string("0x22B8"));
    os.reset();
}

void testLogStreamStrings() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();

    os << "Hello ";
    myassert(buf.toString() == string("Hello "));

    string chenshuo = "Shuo Chen";
    os << chenshuo;
    myassert(buf.toString() == string("Hello Shuo Chen"));
}

void testLogStreamLong() {
    LogStream os;
    const LogStream::Buffer &buf = os.getBuffer();
    for (int i = 0; i < 399; ++i) {
        os << "123456789 ";
        myassert(buf.length() == 10 * (i + 1));
        myassert(buf.remain() == 4000 - 10 * (i + 1));
    }

    os << "abcdefghi ";
    myassert(buf.length() == 3990);
    myassert(buf.remain() == 10);

    os << "abcdefghi";
    myassert(buf.length() == 3999);
    myassert(buf.remain() == 1);
}


int main() {
    testLogStreamBooleans();
    testLogStreamIntegers();
    testLogStreamIntegerLimits();
    testLogStreamFloats();
    testLogStreamVoid();
    testLogStreamStrings();
    testLogStreamLong();
    return 0; 
}