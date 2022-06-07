#ifndef HATRED_PROTO_H
#define HATRED_PROTO_H

#include <string>

namespace hatred::proto {
    int recv_int(int sock);
     
    struct hatred_hdr {
        int length;
        int op;

        static int recv(int sock, hatred_hdr& to);
        
        int send(int sock);
    };
}

#endif