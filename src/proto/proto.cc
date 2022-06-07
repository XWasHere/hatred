#include <sys/socket.h>
#include <arpa/inet.h>

#include "./proto.h"

namespace hatred::proto {
    int recv_int(int sock, int& to) {
        int dat;
        if (recv(sock, &dat, 4, 0) <= 0) return -1;
        to = ntohl(dat);
        return 0;
    }

    int send_int(int sock, int value) {
        int dat = htonl(value);
        if (send(sock, &dat, 4, 0) < 0) return -1;
        return 0;
    }

    int hatred_hdr::recv(int sock, hatred_hdr& to) {
        if (recv_int(sock, to.length)) return -1;
        if (recv_int(sock, to.op))     return -1;

        return 0;
    }

    int hatred_hdr::send(int sock) {
        if (send_int(sock, length)) return -1;
        if (send_int(sock, op))     return -1;

        return 0;
    }
}