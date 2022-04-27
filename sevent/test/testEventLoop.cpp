#include<iostream>
#include "../net/EventLoop.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

int main(){
    EventLoop loop;
    
    thread t([&loop]() { loop.loop(); });

    t.join();

    return 0;
}