#include<iostream>
#include<string.h>
#include "../EventLoop.h"
#include "../../base/Timestamp.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;

int cnt = 0;
EventLoop *g_loop = nullptr;
void timeout(const char  *msg){
    cout<<msg<<" - "<<Timestamp::now().toString()<<endl;

    if (++cnt == 10) {
        g_loop->quit();
    }
}

int main(){
    EventLoop loop;
    g_loop = &loop;
    //2 3 1 1.5 2.5 3 3.5 3 3 3
    cout << "start - " << Timestamp::now().toString() << endl;
    TimerId id = loop.addTimer(0, [] { timeout("every 2"); }, 2000);
    TimerId id2 = loop.addTimer(1000, [] { timeout("once 1"); });
    loop.addTimer(1500, [&] {
        timeout("once 1.5");
        g_loop->cancelTimer(id);
        g_loop->cancelTimer(id2);
    });
    loop.addTimer(2500, [] { timeout("once 2.5");});
    loop.addTimer(3500, [] { timeout("once 3.5"); });
    loop.addTimer(0, [] { timeout("every 3"); }, 3000);


    loop.loop();

    return 0;
}