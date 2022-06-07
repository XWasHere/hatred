#ifndef HATRED_PROTO_H
#define HATRED_PROTO_H

#include <string>

namespace hatred::proto {
    int recv_int(int sock, int& to);
    int send_int(int sock, int value);
    
    int recv_string(int sock, std::string& to);
    int send_string(int sock, const std::string& value);

    enum class hatred_op {
        ECHO
    };

    struct hatred_hdr {
        int length;
        int op;

        static int recv(int sock, hatred_hdr& to);
        int send(int sock);
    };

    struct hatred_echo {
        std::string message;

        static int recv(int sock, hatred_echo& to);
        int send(int sock);
    };
}

#endif