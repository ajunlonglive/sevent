#ifndef SEVENT_BASE_COMMONUTIL_H
#define SEVENT_BASE_COMMONUTIL_H

#include <string>

namespace sevent{

namespace CommonUtil{
    std::string getHostname();
    pid_t getPid();

} // namespace CommonUtil
} // namespace sevent

#endif