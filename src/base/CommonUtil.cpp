#include "CommonUtil.h"
#include <unistd.h>

using namespace std;
using namespace sevent;

namespace sevent{

namespace CommonUtil{

string getHostname() {
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    } else {
        return "unknownhost";
    }
}
pid_t getPid() { return getpid(); }
    
} // namespace CommonUtil
} // namespace sevent

