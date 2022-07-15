#pragma once

#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#define SO_REUSEPORT SO_REUSEADDR

#define SHUT_RD   SD_RECEIVE
#define SHUT_WR   SD_SEND
#define SHUT_RDWR SD_BOTH

typedef int socklen_t;

inline const char* inet_ntop(int af, const void* src, char* dst, socklen_t size) {
    return InetNtop(af, src , dst, size);
}

inline int setsockopt(int socket, int level, int option_name, const void* option_value, socklen_t option_len) {
    return setsockopt((SOCKET)socket, level, option_name, (const char*)option_value, option_len);
}

inline int pipe(int filedes[2]) {
    return _pipe(filedes, 4096, _O_BINARY);
}
#endif