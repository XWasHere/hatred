#pragma once
#define _STRINGIFY(cum) #cum
#define STRINGIFY(cum) _STRINGIFY(cum)
#define IDENTITY(ass)  ass
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
#define _PRAGMA(str)      _Pragma(STRINGIFY(str))
#define DIAG_IGNORE(name) _PRAGMA(GCC diagnostic ignored name)
#define DIAG_WARN(name)   _PRAGMA(GCC diagnostic warn    name)
#define DIAG_ERROR(name)  _PRAGMA(GCC diagnostic error   name)
#define DIAG_PUSH()       _Pragma("GCC diagnostic push")
#define DIAG_POP()        _Pragma("GCC diagnostic pop")
#define DIAG_APPLY(...)   DIAG_PUSH() __VA_ARGS__ DIAG_POP()

namespace hatred::util {
    template<class T, T V> T select_overload = V;
}