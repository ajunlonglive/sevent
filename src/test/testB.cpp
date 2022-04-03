#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "../base/Logger.h"
#include "../base/CommonUtil.h"
#include "../base/LogStream.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <unistd.h>
#include <assert.h>


using namespace std;
using namespace sevent;

class B;
class A  {
public:
    A() { cout << "A()" << endl; }
    A(const A &) { cout << "A copy" << endl; }
    ~A() { cout << "~A()" << endl; }
    // friend ostream & operator<<(ostream & os, A &a) { return os; }
};

class B {
public:
    const static int i = 5;
    void foo(){}
};

void func(unique_ptr<B> &b){}

int main() {
    char s[] = "123\n";
    char s2[] = "456\n";
    char s3[] = "789\n";
    FILE *file;
    file = fopen("a.txt", "a");
    assert(file != nullptr);
    fwrite(s, 1, sizeof(s) - 1, file);
    // fclose(file);


    file = fopen("a.txt", "a");
    assert(file != nullptr);
    fwrite(s2, 1, sizeof(s2) - 1, file);
    file = fopen("a.txt", "a");
    fwrite(s3, 1, sizeof(s3) - 1, file);
    fclose(file);

    // const char *name = basename("./build/a.txt");
    // strrchr(file, '/');
    // cout<<name<<endl;

    // char buf[128]{0};
    // int unit = 24 * 3600;
    // time_t now = time(0);
    // time_t start = now / unit * unit;
    // struct tm tm_time;
    // gmtime_r(&start, &tm_time);
    // strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", &tm_time);
    // cout<<buf<<endl;

    

    return 0;
}