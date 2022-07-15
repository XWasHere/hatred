#pragma once
#define __STRINGIFY(cum) #cum
#define STRINGIFY(cum) __STRINGIFY(cum)
#ifdef DEBUG
#    define DPRINTF(...) printf(__VA_ARGS__)
#else
#    define DPRINTF(...)
#endif
#ifdef DEBUG
#    define DPERROR(...) perror(__VA_ARGS__)
#else
#   define DPERROR(...)
#endif
namespace hatred::util {
    template<class T, T V> T select_overload = V;
}