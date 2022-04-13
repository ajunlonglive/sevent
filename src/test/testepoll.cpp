#include <iostream>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "../base/logger.h"

#include <vector>

using namespace std;

int main(){
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    assert(fd != -1);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // int ret = inet_pton(AF_INET, , &addr.sin_addr);
    // assert(ret == 1);
    
    int option = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
    assert(ret == 0);

    ret = bind(fd, (sockaddr*)&addr,sizeof(addr));
    assert(ret == 0);
    ret = listen(fd, 5);
    assert(ret == 0);

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    assert(epfd > 0);
    epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    event.data.fd = fd;


    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    assert(ret == 0);

    vector<epoll_event> events(16);
    while (true) {
        ret = epoll_wait(epfd, &*events.begin(), events.size(), -1);
        assert(ret != -1);
        for (int i = 0; i < ret; ++i) {
            epoll_event &ev = events[i];
            if (ev.events & EPOLLIN){
                cout<<"EPOLLIN"<<endl;
                if (ev.data.fd == fd){
                    sockaddr_in addrcli;
                    socklen_t addrlen = sizeof(addrcli);
                    int clifd =
                        accept4(fd, (sockaddr *)&addrcli, &addrlen, SOCK_NONBLOCK);
                    assert(clifd > 0);
                    epoll_event eventcli;
                    eventcli.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    eventcli.data.fd = clifd;
                    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &eventcli);
                    assert(ret == 0);
                    cout<<"server accpet"<<endl;
                }else{
                    cout<<"recv msg"<<endl;
                }
            } 
            if (ev.events & EPOLLOUT) {
                if (ev.data.fd == fd)
                    cout<<"server EPOLLOUT"<<endl;
                else{
                    cout<<"client EPOLLOUT"<<endl;
                }
            }
        }
    }

    return 0;
}