#include <sys/socket.h>
#include <arpa/inet.h>

#include "./proto.h"

namespace hatred::proto {
    int recv_int(int sock, int& to) {
        int dat;
        if (recv(sock, &dat, 4, 0) < 0) return -1;
        to = ntohl(dat);
        return 0;
    }

    int hatred_msg::recv(int sock, hatred_msg& to) {
        if (recv_int(sock, to.length) < 0) return -1;
        to.data   = "";

        return 0;
    }
}