#pragma once

namespace hatred::net {
    int readto(int fd, void* buf, int count, int timeout);
};