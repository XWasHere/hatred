#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <charconv>

#include "../proto/proto.h"

using namespace hatred;

enum class op {
    OP_NONE, OP_ECHO
};

int main(int argc, const char** argv) {
    op op = op::OP_NONE;
    
    bool help = 0;
    
    const char* ip_raw = 0;
    const char* port_raw = 0;

    sockaddr_in target_addr = {
        .sin_family = AF_INET,
    };
    
    const char* echo_string = 0;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (arg[0] == '-' && arg[1] == '-') {
            arg = arg + 2;

            if (strcmp(arg, "help") == 0) {
                help = 1;
            } else if (strcmp(arg, "ip") == 0) {
                ip_raw = argv[++i];
                continue;
            } else if (strcmp(arg, "port") == 0) {
                port_raw = argv[++i];
                continue;
            } else if (strcmp(arg, "echo") == 0) {
                if (op != op::OP_NONE) {
                    fprintf(stderr, "cant specify multiple ops");
                    exit(1);
                }

                op = op::OP_ECHO;
                echo_string = argv[++i];
                continue;
            } else {
                fprintf(stderr, "unknown option --%s\n", arg);
                exit(1);
            }
        }
    }

    if (help) {
        printf("usage: %s [--ip <ip>] [--port <port>] (--echo <string>)\n", argv[0]);
        exit(0);
    }

    if (ip_raw) {
        if (!inet_pton(AF_INET, ip_raw, &target_addr.sin_addr)) {
            fprintf(stderr, "ip %s is not valid\n", ip_raw);
            exit(1);
        }
    } else {
        fprintf(stderr, "ip not specified\n");
        exit(1);
    }

    if (port_raw) {
        if (std::from_chars(port_raw, port_raw + strlen(port_raw), target_addr.sin_port).ec != std::errc()) {
            fprintf(stderr, "port %s is not valid\n", port_raw);
            exit(1);
        } else target_addr.sin_port = htons(target_addr.sin_port);
    } else {
        fprintf(stderr, "port not specifed\n");
        exit(1);
    }

    if (op != op::OP_NONE) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(sock, (sockaddr*)&target_addr, sizeof(target_addr))) {
            perror("connect()");
            exit(1);
        }

        if (op == op::OP_ECHO) {
            proto::hatred_hdr{
                .length = 0,
                .op     = 0
            }.send(sock);
        }

        close(sock);
    } else {
        fprintf(stderr, "nothing to do\n");
        exit(1);
    }

    return 0;
}