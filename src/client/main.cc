#include "../common/config.h"

#include <filesystem>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <termio.h>
static cc_t _ECHO = ECHO;
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
    OP_NONE, OP_ECHO, OP_EXEC, OP_LS, OP_GET, OP_PUT, OP_DEL
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

    const char*  fop_path    = 0;
    const char*  fop_path2   = 0;

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
            } else if (strcmp(arg, "ls") == 0) {
                if (op != op::OP_NONE) {
                    fprintf(stderr, "cant specify multiple ops\n");
                    exit(1);
                }
                
                op = op::OP_LS;
                fop_path = argv[++i];
                continue;
            } else if (strcmp(arg, "get") == 0) {
                if (op != op::OP_NONE) {
                    fprintf(stderr, "cant specify multiple ops\n");
                    exit(1);
                }
                
                op = op::OP_GET;
                fop_path = argv[++i];
                fop_path2 = argv[++i];
                continue;
            } else if (strcmp(arg, "put") == 0) {
                if (op != op::OP_NONE) {
                    fprintf(stderr, "cant specify multiple ops\n");
                    exit(1);
                }
                
                op = op::OP_PUT;
                fop_path = argv[++i];
                fop_path2 = argv[++i];
                continue;
            } else if (strcmp(arg, "rm") == 0) {
                if (op != op::OP_NONE) {
                    fprintf(stderr, "cant specify multiple ops\n");
                    exit(1);
                }
                
                op = op::OP_DEL;
                fop_path = argv[++i];
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
        printf("usage: %s [--ip <ip>] [--port <port>] (OP)\n"
               "ops:\n"
               "    --echo <string>\n"
               "    --exec <path> [-- <args>]\n"
               "    --ls   <path>\n"
               "    --get  <path> <to>\n"
               "    --put  <path> <from>\n"
               "    --rm   <path>\n"
               "\n", argv[0]);
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

        // printf("%s:%s\n", ip_raw, port_raw);
        
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
                    args[i - extra_argc] = std::string(argv[i]);
                }
            }

            for (std::string& a : args) printf("%s\n", a.c_str());
    
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
            curr_tios.c_lflag &= ~(ICANON);
            curr_tios.c_cc[VINTR] = _POSIX_VDISABLE;
            
            tcsetattr(0, TCSANOW, &curr_tios);

            atexit(fnutil::decay<void(), 0xC1000001>([&]{
                tcsetattr(0, TCSANOW, &orig_tios);
                if (streams[1].fd != -1) {
                    close(streams[1].fd);
                    streams[1].fd = -1;
                }
            }));

            while (poll(streams, 2, 1000) != -1) {
                if (streams[0].revents & POLLIN) {
                    char buf[128] = {};
                    int read = ::read(0, buf, 128);

                    for (int i = 0; i < read; i++) if (buf[i] == 3) {
                        close(sock);
                        exit(0);
                    }

                    proto::hatred_hdr{
                        .length = 0,
                        .op = (int)proto::hatred_op::STREAM
                    }.send(sock);

                    proto::hatred_stream{
                        .fno = 0,
                        .data = std::string(buf)
                    }.send(sock);
                }

                if (streams[1].revents & POLLIN) {
                    proto::hatred_hdr header;
                    if (proto::hatred_hdr::recv(sock, header)) exit(0);

                    // printf("== recv %i\n", header.op);

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

                if (streams[1].revents & POLLHUP) {
                    exit(1);
                }
            }
        } else if (op == op::OP_LS) {
            if (fop_path == 0) {
                fprintf(stderr, "path not specified\n");
                close(sock);
                exit(1);
            }

            proto::hatred_hdr{
                .length = 0,
                .op     = (int)proto::hatred_op::GETDIR
            }.send(sock);

            proto::hatred_getdir{
                .name   = std::string(fop_path)
            }.send(sock);

            proto::hatred_hdr res;
            if (!proto::hatred_hdr::recv(sock, res)) {
                if (proto::hatred_op(res.op) == proto::hatred_op::DIRDT) {
                    proto::hatred_dir info;
                    if (!proto::hatred_dir::recv(sock, info)) {
                        for (const auto& f : info.content) {
                            char t = '?';

                            switch (proto::hatred_ftype(f.type)) {
                                case proto::hatred_ftype::FILE: t = '-'; break;
                                case proto::hatred_ftype::DIR:  t = 'd'; break;
                            }

                            printf("%c????????? ? ? ? ? ? %s\n", t, f.name.c_str());
                        }
                    }
                } else if (proto::hatred_op(res.op) == proto::hatred_op::ERROR) {
                    proto::hatred_error err;
                    if (!proto::hatred_error::recv(sock, err)) {
                        fprintf(stderr, "%s\n", proto::hatred_strerror(err.what).c_str());
                    }
                }
            }
        } else if (op == op::OP_GET) {
            if (fop_path == 0) {
                fprintf(stderr, "path not specified\n");
                close(sock);
                exit(1);
            }

            if (fop_path2 == 0) {
                fprintf(stderr, "output path not specified\n");
                close(sock);
                exit(1);
            }

            FILE* out = fopen(fop_path2, "w");
            if (!out) {
                perror("fopen()");
                close(sock);
                exit(1);
            }

            proto::hatred_hdr{
                .length = 0,
                .op     = (int)proto::hatred_op::GETFILE
            }.send(sock);

            proto::hatred_getfile{
                .name   = std::string(fop_path)
            }.send(sock);

            proto::hatred_hdr res;
            if (!proto::hatred_hdr::recv(sock, res)) {
                if (proto::hatred_op(res.op) == proto::hatred_op::ACK) {
                    while (1) {
                        if (!proto::hatred_hdr::recv(sock, res)) {
                            if (proto::hatred_op(res.op) == proto::hatred_op::STREAM) {
                                proto::hatred_stream sd;
                                if (!proto::hatred_stream::recv(sock, sd)) {
                                    if (sd.data.size() == 0) break;
                                    if (fwrite(sd.data.c_str(), 1, sd.data.size(), out) == -1) {
                                        perror("fwrite()");
                                        close(sock);
                                        fclose(out);
                                        std::filesystem::remove(fop_path2);
                                        exit(1);
                                    }
                                } else break;
                            } else break;
                        } else break;
                    }

                    fclose(out);
                } else if (proto::hatred_op(res.op) == proto::hatred_op::ERROR) {
                    proto::hatred_error err;
                    if (!proto::hatred_error::recv(sock, err)) {
                        fprintf(stderr, "%s\n", proto::hatred_strerror(err.what).c_str());
                        fclose(out);
                        std::filesystem::remove(fop_path2);
                    }
                }
            }
        } else if (op == op::OP_PUT) {
            if (fop_path == 0) {
                fprintf(stderr, "path not specified\n");
                close(sock);
                exit(1);
            }

            if (fop_path2 == 0) {
                fprintf(stderr, "input path not specified\n");
                close(sock);
                exit(1);
            }

            FILE* file = fopen(fop_path2, "r");

            if (!file) {
                perror("fopen()");
                close(sock);
                exit(1);
            }

            proto::hatred_hdr{
                .length = 0,
                .op     = (int)proto::hatred_op::PUTFILE
            }.send(sock);

            proto::hatred_putfile{
                .name   = std::string(fop_path)
            }.send(sock);

            proto::hatred_hdr res;
            if (!proto::hatred_hdr::recv(sock, res)) {
                if (proto::hatred_op(res.op) == proto::hatred_op::ACK) {
                    char* chunk = new char[STREAM_CHUNK_SIZE];

                    while (size_t csiz = fread(chunk, 1, STREAM_CHUNK_SIZE, file)) {
                        proto::hatred_hdr{.length = 0, .op = (int)proto::hatred_op::STREAM}.send(sock);
                        proto::hatred_stream{.fno = 0, .data = std::string(chunk, csiz)}.send(sock);
                    }

                    delete[] chunk;

                    proto::hatred_hdr{.length = 0, .op = (int)proto::hatred_op::STREAM}.send(sock);
                    proto::hatred_stream{.fno = 0, .data = ""}.send(sock);

                    fclose(file);
                } else if (proto::hatred_op(res.op) == proto::hatred_op::ERROR) {
                    proto::hatred_error err;
                    if (!proto::hatred_error::recv(sock, err)) {
                        fprintf(stderr, "%s\n", proto::hatred_strerror(err.what).c_str());
                    }
                }
            }
        } else if (op == op::OP_DEL) {            
            if (fop_path == 0) {
                fprintf(stderr, "path not specified\n");
                close(sock);
                exit(1);
            }

            proto::hatred_hdr{
                .length = 0,
                .op     = (int)proto::hatred_op::RMFILE
            }.send(sock);

            proto::hatred_rmfile{
                .name   = std::string(fop_path)
            }.send(sock);

            proto::hatred_hdr res;
            if (!proto::hatred_hdr::recv(sock, res)) {
                if (proto::hatred_op(res.op) == proto::hatred_op::ACK) {
                    printf("deleted \"%s\"\n", fop_path);
                } else if (proto::hatred_op(res.op) == proto::hatred_op::ERROR) {
                    proto::hatred_error err;
                    if (!proto::hatred_error::recv(sock, err)) {
                        fprintf(stderr, "%s\n", proto::hatred_strerror(err.what).c_str());
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