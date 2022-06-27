#include <poll.h>
#include <unistd.h>

#include "./net.h"

int hatred::net::readto(int fd, void* buf, int count, int timeout) {
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
}