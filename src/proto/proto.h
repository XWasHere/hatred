#ifndef HATRED_PROTO_H
#define HATRED_PROTO_H

#include <string>

namespace hatred::proto {
    int recv_int(int sock);
     
    struct hatred_msg {
        int   length;
        char* data;

        static int recv(int sock, hatred_msg& to);
        
        int send(int sock);
    };
}

#endif