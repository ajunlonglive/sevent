#include "Poller.h"
#include "../base/Logger.h"
#include <errno.h>

using namespace sevent;
using namespace sevent::net;


int Poller::poll(int timeout) {
    activeChannels.clear();
    int count = doPoll(timeout);
    int saveErr = errno;
    pollTime = Timestamp::now();
    if (count > 0) {
        fillActiveChannels(count);
    } else if (count == 0) {
        LOG_TRACE << "timed out";
    } else {
        if (saveErr != EINTR){
            errno = saveErr;
            LOG_SYSERR << "Poller::poll()";
        }
    }
    return count;
}