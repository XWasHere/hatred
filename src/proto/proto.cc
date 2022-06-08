#include <sys/socket.h>
#include <arpa/inet.h>

#include <string.h>
#include <errno.h>

#include "./proto.h"

#ifdef DEBUG
#define DDIE(x) { perror(x); return -1; }
#else
#define DDIE(x) { return -1; }
#endif

namespace hatred::proto {
    int recv_string(int sock, std::string& to) {
        int length;
        char* value;

        if (recv_int(sock, length)) return -1;

        if (length == 0) {
            to = std::string("");
            return 0;
        } else if (length < 128) {
            value = (char*)alloca(length + 1);
            memset(value, 0, length + 1);
            if (recv(sock, value, length, 0) <= 0) return -1;
            to = std::string(value);
            return 0;
        } else {
            value = (char*)malloc(length + 1);
            memset(value, 0, length + 1);
            if (recv(sock, value, length, 0) <= 0) {
                free(value);
                return -1;
            }
            to = std::string(value);
            free(value);
            return 0;
        }
    }
    
    int send_string(int sock, const std::string& value) {
        if (send_int(sock, value.length()))                     return -1;
        if (send(sock, value.c_str(), value.length(), 0) == -1) return -1;

        return 0;
    }

    int recv_int(int sock, int& to) {
        int dat;
        if (recv(sock, &dat, 4, 0) <= 0) DDIE("recv");
        to = ntohl(dat);
        return 0;
    }

    int send_int(int sock, int value) {
        int dat = htonl(value);
        if (send(sock, &dat, 4, 0) < 0) DDIE("send");
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

    int hatred_echo::recv(int sock, hatred_echo& to) {
        if (recv_string(sock, to.message)) return -1;

        return 0;
    }

    int hatred_echo::send(int sock) {
        if (send_string(sock, message)) return -1;

        return 0;
    }

    int hatred_exec::recv(int sock, hatred_exec& to) {
        if (recv_string(sock, to.cmd)) return -1;
        if (recv_vector<std::string, recv_string>(sock, to.args)) return -1;

        return 0;
    }

    int hatred_exec::send(int sock) {
        if (send_string(sock, cmd)) return -1;
        if (send_vector<std::string, send_string>(sock, args)) return -1;

        return 0;
    }

    int hatred_stream::recv(int sock, hatred_stream& to) {
        if (recv_int(sock, to.fno)) return -1;
        if (recv_string(sock, to.data)) return -1;

        return 0;
    }

    int hatred_stream::send(int sock) {
        if (send_int(sock, fno)) return -1;
        if (send_string(sock, data)) return -1;

        return 0;
    }
}