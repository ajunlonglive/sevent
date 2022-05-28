#include "sevent/base/CommonUtil.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#include <strings.h>
#else
#include <winsock2.h>
#include <processthreadsapi.h>
#endif

using namespace std;
using namespace sevent;

namespace sevent{

namespace CommonUtil{

string getHostname() {
    char buf[256];
    if (::gethostname(buf, sizeof(buf)) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    } else {
        return "unknownhost";
    }
}
int getPid() {
    #ifndef _WIN32
    return ::getpid(); 
    #else
    return ::GetCurrentProcessId();
    #endif
}

struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    #ifndef _WIN32
    return ::gmtime_r(timep, result);
    #else
    ::gmtime_s(result, timep);
    return nullptr;
    #endif
}

size_t fwrite_unlocked(const void *ptr, size_t size, size_t n, FILE *stream) {
    #ifndef _WIN32
    return ::fwrite_unlocked(ptr, size, n, stream);
    #else
    return ::fwrite(ptr, size, n, stream);
    #endif
}
void chomp(char *s) {
	size_t len;
	if (s && (len = strlen (s)) > 0 && s[len - 1] == '\n') {
		s[--len] = 0;
		if (len > 0 && s[len - 1] == '\r')
			s[--len] = 0;
	}
}

const char *strerror_tl(int errnum) {
    #ifndef _WIN32
    thread_local char errnoBuf[512];
    return strerror_r(errnum, errnoBuf, sizeof(errnoBuf));
    #else
    thread_local string msgString;
    char *msgBuf = nullptr;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errnum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<char *>(&msgBuf),
        0, NULL );
    // MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
    chomp(msgBuf);
    msgString = string(msgBuf);// + "(errno=" + to_string(errnum) + ")";
    return msgString.c_str();
    #endif
}

int stricmp(const char *l, const char *r) {
    #ifndef _WIN32
    return strcasecmp(l, r);
    #else
    return _stricmp(l, r);
    #endif
}
    
} // namespace CommonUtil
} // namespace sevent

