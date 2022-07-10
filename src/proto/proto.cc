#ifndef __WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <string.h>
#include <errno.h>

#include "./proto.h"

#ifdef DEBUG
#ifndef __WIN32
#define DDIE(x) { perror(x); return -1; }
#else
#define DDIE(x) { printf(x ":%i\n", WSAGetLastError()); return -1; }
#endif
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
            if (recv(sock, (char*)value, length, 0) <= 0) return -1;
            to = std::string(value);
            return 0;
        } else {
            value = (char*)malloc(length + 1);
            memset(value, 0, length + 1);
            if (recv(sock, (char*)value, length, 0) <= 0) {
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
        if (recv(sock, (char*)&dat, 4, 0) <= 0) DDIE("recv");
        to = ntohl(dat);
        return 0;
    }

    int send_int(int sock, int value) {
        int dat = htonl(value);
        if (send(sock, (char*)&dat, 4, 0) < 0) DDIE("send");
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

    int hatred_finfo::recv(int sock, hatred_finfo &to) {
        if (recv_int(sock, to.type))    return -1;
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_finfo::send(int sock) {
        if (send_int(sock, type))    return -1;
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_dir::recv(int sock, hatred_dir &to) {
        if (recv_vector<hatred_finfo>(sock, to.content)) return -1;

        return 0;
    }

    int hatred_dir::send(int sock) {
        if (send_vector<hatred_finfo>(sock, content)) return -1;

        return 0;
    }
    
    int hatred_file::recv(int sock, hatred_file &to) {
        if (recv_string(sock, to.content)) return -1;

        return 0;
    }

    int hatred_file::send(int sock) {
        if (send_string(sock, content)) return -1;

        return 0;
    }

    int hatred_getfinfo::recv(int sock, hatred_getfinfo &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_getfinfo::send(int sock) {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_getdir::recv(int sock, hatred_getdir &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_getdir::send(int sock) {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_mkdir::recv(int sock, hatred_mkdir &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_mkdir::send(int sock) {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_rmdir::recv(int sock, hatred_rmdir &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_rmdir::send(int sock) {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_getfile::recv(int sock, hatred_getfile &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_getfile::send(int sock) {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_putfile::recv(int sock, hatred_putfile &to) {
        if (recv_string(sock, to.name)) return -1;
        if (hatred_file::recv(sock, to.file)) return -1;

        return 0;
    }

    int hatred_putfile::send(int sock) {
        if (send_string(sock, name)) return -1;
        if (file.send(sock)) return -1;

        return 0;
    }

    int hatred_rmfile::recv(int sock, hatred_rmfile &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_rmfile::send(int sock) {
        if (send_string(sock, name)) return -1;

        return 0;
    }
}