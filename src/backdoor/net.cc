#ifndef __WIN32
#include <poll.h>
#else
#include <windows.h>
#endif
#include <unistd.h>

#include "./net.h"

int hatred::net::readto(int fd, void* buf, int count, int timeout) {
#ifndef __WIN32
    pollfd check = {
        .fd      = fd,
        .events  = POLLIN,
        .revents = 0
    };

    int res = poll(&check, 1, timeout);

    if (res > 0) {
        if (check.revents & POLLIN) return read(fd, buf, count);
    } else return res;
    return 0;
#else
    return recv(fd, buf, count, 0);
#endif
}