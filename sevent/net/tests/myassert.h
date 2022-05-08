#include <iostream>
#include <string.h>

#define myassert(expr)                                                         \
    (static_cast<bool>(expr) ? void(0)                                         \
                             : myassertf(#expr, __FILE__, __func__, __LINE__))

void myassertf(const char *expr, const char *file, const char *func, int line) {
    const char *slash = strrchr(file, '/');
    if (slash)
        file = slash + 1;
    std::cout << file << ":" << line << ": " << func << ": Assertion `" << expr
         << "` failed" << std::endl;
    abort();
}