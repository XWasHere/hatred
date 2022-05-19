#ifndef HATRED_NET_H
#define HATRED_NET_H

namespace hatred::net {
    int readto(int fd, void* buf, int count, int timeout);
};

#endif