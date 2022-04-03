#ifndef SEVENT_BASE_COMMONUTIL_H
#define SEVENT_BASE_COMMONUTIL_H

#include <string>

namespace sevent{

class CommonUtil{
public:
    static std::string getHostname();
    static pid_t getPid();
};

} // namespace sevent

#endif