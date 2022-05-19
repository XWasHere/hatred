#ifndef HATRED_UTIL_H
#   define HATRED_UTIL_H
#   define __STRINGIFY(cum) #cum
#   define STRINGIFY(cum) __STRINGIFY(cum)
#   ifdef DEBUG
#       define DPRINTF(...) printf(__VA_ARGS__)
#   else
#       define DPRINTF(...)
#   endif
#endif