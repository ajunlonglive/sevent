#include "sevent/net/Buffer.h"
#include "myassert.h"
#include <iostream>
#include <string.h>

using namespace std;
using namespace sevent::net;

void testAppendAndRead() {
    Buffer buf;
    myassert(buf.readableBytes() == 0);
    myassert(buf.writableBytes() == Buffer::initialSize);
    myassert(buf.prependableBytes() == Buffer::prepends);

    const string str(200, 'x');
    buf.append(str);
    myassert(buf.readableBytes() == str.size());
    myassert(buf.writableBytes() == Buffer::initialSize - str.size());
    myassert(buf.prependableBytes() == Buffer::prepends);

    const string str2 = buf.readAsString(50);
    myassert(str2.size() == 50);
    myassert(str2 == string(50, 'x'));
    myassert(buf.readableBytes() == (str.size() - str2.size()));
    myassert(buf.writableBytes() == Buffer::initialSize - str.size());
    myassert(buf.prependableBytes() == Buffer::prepends + str2.size());

    buf.append(str);
    myassert(buf.readableBytes() == (2 * str.size() - str2.size()));
    myassert(buf.writableBytes() == Buffer::initialSize - 2 * str.size());
    myassert(buf.prependableBytes() == Buffer::prepends + str2.size());

    const string str3 = buf.readAllAsString();
    myassert(str3.size() == 350);
    myassert(str3 == string(350, 'x'));
    myassert(buf.readableBytes() == 0);
    myassert(buf.writableBytes() == Buffer::initialSize);
    myassert(buf.prependableBytes() == Buffer::prepends);
}

void testGrow() {
    Buffer buf;
    buf.append(string(400, 'y'));
    myassert(buf.readableBytes() == 400);
    myassert(buf.writableBytes() == Buffer::initialSize - 400);

    buf.retrieve(50);
    myassert(buf.readableBytes() == 350);
    myassert(buf.writableBytes() == Buffer::initialSize - 400);
    myassert(buf.prependableBytes() == Buffer::prepends + 50);

    buf.append(string(1000, 'z'));
    myassert(buf.readableBytes() == 1350);
    myassert(buf.writableBytes() == 0);
    myassert(buf.prependableBytes() == Buffer::prepends + 50); // FIXME

    buf.retrieveAll();
    myassert(buf.readableBytes() == 0);
    myassert(buf.writableBytes() == 1400); // FIXME
    myassert(buf.prependableBytes() == Buffer::prepends);
}

void testInsideGrow() {
    Buffer buf;
    buf.append(string(800, 'y'));
    myassert(buf.readableBytes() == 800);
    myassert(buf.writableBytes() == Buffer::initialSize - 800);

    buf.retrieve(500);
    myassert(buf.readableBytes() == 300);
    myassert(buf.writableBytes() == Buffer::initialSize - 800);
    myassert(buf.prependableBytes() == Buffer::prepends + 500);

    buf.append(string(300, 'z'));
    myassert(buf.readableBytes() == 600);
    myassert(buf.writableBytes() == Buffer::initialSize - 600);
    myassert(buf.prependableBytes() == Buffer::prepends);
}

void testShrink() {
    Buffer buf;
    buf.append(string(2000, 'y'));
    myassert(buf.readableBytes() == 2000);
    myassert(buf.writableBytes() == 0);
    myassert(buf.prependableBytes() == Buffer::prepends);

    buf.retrieve(1500);
    myassert(buf.readableBytes() == 500);
    myassert(buf.writableBytes() == 0);
    myassert(buf.prependableBytes() == Buffer::prepends + 1500);

    buf.shrinkToFit();
    myassert(buf.readableBytes() == 500);
    myassert(buf.writableBytes() == Buffer::initialSize - 500);
    myassert(buf.readAllAsString() == string(500, 'y'));
    myassert(buf.prependableBytes() == Buffer::prepends);
}

void testPrepend() {
    Buffer buf;
    buf.append(string(200, 'y'));
    myassert(buf.readableBytes() == 200);
    myassert(buf.writableBytes() == Buffer::initialSize - 200);
    myassert(buf.prependableBytes() == Buffer::prepends);

    int x = 0;
    buf.prepend(&x, sizeof(x));
    myassert(buf.readableBytes() == 204);
    myassert(buf.writableBytes() == Buffer::initialSize - 200);
    myassert(buf.prependableBytes() == Buffer::prepends - 4);
}

void testReadInt() {
    Buffer buf;
    buf.append("HTTP");

    myassert(buf.readableBytes() == 4);
    myassert(buf.peekInt8() == 'H');
    int top16 = buf.peekInt16();
    myassert(top16 == 'H' * 256 + 'T');
    myassert(buf.peekInt32() == top16 * 65536 + 'T' * 256 + 'P');

    myassert(buf.readInt8() == 'H');
    myassert(buf.readInt16() == 'T' * 256 + 'T');
    myassert(buf.readInt8() == 'P');
    myassert(buf.readableBytes() == 0);
    myassert(buf.writableBytes() == Buffer::initialSize);

    buf.appendInt8(-1);
    buf.appendInt16(-2);
    buf.appendInt32(-3);
    myassert(buf.readableBytes() == 7);
    myassert(buf.readInt8() == -1);
    myassert(buf.readInt16() == -2);
    myassert(buf.readInt32() == -3);
}

void testEOL() {
    Buffer buf;
    buf.append(string(100000, 'x'));
    const char *null = NULL;
    myassert(buf.findEOL() == null);
    myassert(buf.findEOL(buf.peek() + 90000) == null);
}

int main() {
    testAppendAndRead();
    testGrow();
    testInsideGrow();
    testShrink();
    testPrepend();
    testReadInt();
    testEOL();
    cout << "finish Buffer_test" << endl;
    return 0;
}