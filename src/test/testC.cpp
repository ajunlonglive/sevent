#include<iostream>
#include "../base/Logger.h"
#include <sys/resource.h>
#include<string.h>

using namespace std;
using namespace sevent;

void func(const string &str){
    Logger &logger = Logger::instance();
    logger.log(__FILE__, __LINE__, Logger::INFO, str);
}

void foo(const string &str){
    for (int i = 0; i < 5; ++i) {
        func(str);
    }
}
int main(){
    // size_t kOneGB = 1000*1024*1024;
    // rlimit rl = {0};
    // getrlimit(RLIMIT_AS, &rl);
    // cout<<(rl.rlim_cur / kOneGB) <<endl;
    // cout<<(rl.rlim_max / kOneGB) <<endl;
    cout<<"begin"<<endl;
    DefaultLogProcessor::setShowMicroSecond(true);
    unique_ptr<LogAppender> p(new LogAsynAppender);
    Logger &logger = Logger::instance();
    logger.setLogAppender(p);

    thread t1(foo, "t1");
    thread t2(foo, "t2");
    // foo("main");
    this_thread::sleep_for(5s);
    t1.join();
    t2.join();
    cout<<"reset"<<endl;
    logger.getLogAppender().reset();
    
    while(1){
        this_thread::sleep_for(10s);
    }

    return 0;
}