#ifndef SEVENT_BASE_COMMONUTIL_H
#define SEVENT_BASE_COMMONUTIL_H

#include <string>

namespace sevent{

namespace CommonUtil{
    std::string getHostname();
    int getPid();
    struct tm *gmtime_r(const time_t *timep, struct tm *result);
    size_t fwrite_unlocked(const void *ptr, size_t size, size_t n, FILE *stream);
    // for windows
    void chomp(char *s);
    const char *strerror_tl(int errnum);
    int stricmp(const char *l, const char *r);

} // namespace CommonUtil
} // namespace sevent

#endif