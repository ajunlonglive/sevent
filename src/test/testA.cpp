#include <iostream>
#include "../base/CurrentThread.h"
#include "../base/logger.h"
#include <mutex>
#include <string>
#include <thread>
using namespace std;
// using namespace sevent;

mutex m;

// class MyAppender : public LogAppender {
//     void append(const char *log, int len){
//         lock_guard<mutex> lg(m);
//         fwrite(log, 1, len, stdout);
//     }
//     void flush(){
//         fflush(stdout);
//     }
// };

// void func(string &str,int num){
//     {
//         lock_guard<mutex> lg(m);
//         cout<<num<<"-";
//     }
//     Logger &logger = Logger::instance();
//     logger.log(__FILE__, __LINE__, Logger::INFO, str);
// }

// void testOneThreadGetMultiTimes(string &str,int num) {
//     func(str,num);
//     func(str,num);
//     this_thread::sleep_for(1000ms);
//     func(str,num);
//     this_thread::sleep_for(10ms);
//     func(str,num);
//     this_thread::sleep_for(10ms);
//     func(str,num);
//     this_thread::sleep_for(10ms);
//     func(str,num);
// }
int main() {
    // string s = "Hello";
    sevent::DefaultLogProcessor::setShowMicroSecond(true);
    sevent::setUTCOffset(8 * 3600);
    // unique_ptr<LogAppender> p(new MyAppender);
    // Logger &logger = Logger::instance();
    // logger.setLogAppender(p);
    
    // func(s,1);
    // func(s,1);
    // testOneThreadGetMultiTimes(s);

    // thread t1(func,std::ref(s),2);
    // thread t2(func,std::ref(s),2);
    // t1.join();
    // t2.join();


    sevent::Logger::instance().setLogLevel(sevent::Logger::DEBUG);
    LOG_DEBUG << "LOG_DEBUG";
    LOG_INFO << "LOG_INFO";
    LOG_WARN << "LOG_WARN";
    LOG_ERROR << "LOG_ERROR";
    LOG_FATAL << "LOG_FATAL";

    return 0;
}