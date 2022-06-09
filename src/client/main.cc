#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <termio.h>
#undef ECHO

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <charconv>

#include "../proto/proto.h"
#include "../common/fnutil.h"

using namespace hatred;

enum class op {
    OP_NONE, OP_ECHO, OP_EXEC
};

int main(int argc, const char** argv) {
    op op = op::OP_NONE;
    
    bool help = 0;
    
    const char* ip_raw = 0;
    const char* port_raw = 0;

    sockaddr_in target_addr = {
        .sin_family = AF_INET,
    };
    
    const char*  echo_string = 0;
    const char*  exec_string = 0;
    int          extra_argc = 0;

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
                    fprintf(stderr, "cant specify multiple ops\n");
                    exit(1);
                }

                op = op::OP_ECHO;
                echo_string = argv[++i];
                continue;
            } else if (strcmp(arg, "exec") == 0) {
                if (op != op::OP_NONE) {
                    fprintf(stderr, "cant specify multiple ops\n");
                    exit(1);
                }
                
                op = op::OP_EXEC;
                exec_string = argv[++i];
                continue;
            } else if (strcmp(arg, "") == 0) {
                extra_argc = ++i;
                break;
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
            
            proto::hatred_echo{
                .message = echo_string
            }.send(sock);

            proto::hatred_hdr header;
            proto::hatred_hdr::recv(sock, header);

            proto::hatred_echo echo;
            proto::hatred_echo::recv(sock, echo);

            printf("echo: %s\n", echo.message.c_str());
        } else if (op == op::OP_EXEC) {
            std::vector<std::string> args;
    
            if (extra_argc) {
                args.resize(argc - extra_argc);
                for (int i = extra_argc; i < argc; i++) {
                    args.push_back(std::string(argv[i]));
                }
            }

            // for (std::string& a : args) printf("%s\n", a.c_str());
    
            proto::hatred_hdr{
                .length = 0,
                .op     = (int)proto::hatred_op::EXEC
            }.send(sock);

            proto::hatred_exec{
                .cmd  = exec_string,
                .args = args
            }.send(sock);

            pollfd streams[2] = {
                {
                    .fd = 0,
                    .events = POLLIN
                },
                {
                    .fd = sock,
                    .events = POLLIN
                }
            };

            termios orig_tios;
            termios curr_tios;

            tcgetattr(0, &orig_tios);
            
            curr_tios = orig_tios;

            tcsetattr(0, TCSANOW, &curr_tios);

            atexit(fnutil::decay<void(), 0xC1000001>([&]{
                printf("bye\n");
                tcsetattr(0, TCSANOW, &orig_tios);
            }));

            exit(0);

            while (poll(streams, 2, 1000) != -1) {
                if (streams[0].revents & POLLIN) {
                    char buf[128] = {};
                    int read = ::read(0, buf, 128);
                    printf("STDIN: %s\n", buf);
                }

                if (streams[1].revents & POLLIN) {
                    proto::hatred_hdr header;
                    proto::hatred_hdr::recv(sock, header);

                    printf("== recv %i\n", header.op);

                    if (proto::hatred_op(header.op) == proto::hatred_op::STREAM) {
                        proto::hatred_stream body;
                        proto::hatred_stream::recv(sock, body);

                        if (body.fno == 1) {
                            fwrite(body.data.c_str(), 1, body.data.length(), stdout);
                        } else if (body.fno == 2) {
                            fwrite(body.data.c_str(), 1, body.data.length(), stderr);
                        }
                    } else if (proto::hatred_op(header.op) == proto::hatred_op::CLOSE) {
                        break;
                    }
                }
            }
        }

        close(sock);
    } else {
        fprintf(stderr, "nothing to do\n");
        exit(1);
    }

    return 0;
}