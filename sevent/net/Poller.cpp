#include "sevent/net/Poller.h"
#include "sevent/base/Logger.h"
#include "sevent/net/poller/EpollPoller.h"
#include "sevent/net/poller/PollPoller.h"
#include "sevent/net/poller/SelectPoller.h"
#include "sevent/net/SocketsOps.h"
#include <errno.h>

using namespace sevent;
using namespace sevent::net;


int Poller::poll(int timeout) {
    activeChannels.clear();
    int count = doPoll(timeout);
    int saveErr = sockets::getErrno();
    pollTime = Timestamp::now();
    if (count > 0) {
        fillActiveChannels(count);
    } else if (count == 0) {
        // LOG_TRACE << "timed out";
    } else {
        #ifndef _WIN32
        if (saveErr != EINTR){
            errno = saveErr;
            LOG_SYSERR << "Poller::poll()";
        }
        #else
        if (saveErr != WSAEINTR){
            sockets::setErrno(saveErr);
            LOG_SYSERR << "Poller::poll()";
        }
        #endif
    }
    return count;
}

Poller *Poller::newPoller() {
    if (::getenv("SEVENT_USE_SELECT"))
        return new SelectPoller;
    #ifndef _WIN32
    else if (::getenv("SEVENT_USE_POLL"))
        return new PollPoller;
    #endif
    else
        return new EpollPoller;
}
