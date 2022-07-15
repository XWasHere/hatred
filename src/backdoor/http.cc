#include <string>
#include <charconv>

#ifndef __WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "./http.h"
#include "./net.h"
#include "../common/util.h"
//#include "../common/fdstream.h"

namespace hatred::http {
    int connect(int socket, const std::string& url) {
        http_url to = parse_url(url);

        sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port   = htons(to.port),
#ifndef __WIN32
            .sin_addr   = { .s_addr = inet_addr(to.host.c_str()) }
#else
            .sin_addr   = { .S_un = { .S_addr = inet_addr(to.host.c_str()) } }
#endif
        };

        if (::connect(socket, (sockaddr*)&addr, sizeof(addr))) {
            DPERROR("connect()");
            return 1;
        }

        return 0;
    }

    int http_write_message(const http_request& in, std::string& out, unsigned int flags) {
        using namespace flags;

        http_url dest;
        if (!(flags & VBURL)) {
            dest = parse_url(in.url);
            DPRINTF("request to \"http://%s:%i/%s\"\n", dest.host.c_str(), dest.port, dest.path.c_str());
        }

        out += flags & VBURL ? in.url : in.method + ((flags & NOFIX) ? " " : " /") + dest.path + " HTTP/1.1\r\n";
        if (!(flags & VBURL) && !in.header.contains("Host")) out += "Host: " + dest.host + ":" + std::to_string(dest.port) + "\r\n";
        if (!in.header.contains("Content-Length")) out += "Content-Length: " + std::to_string(in.body.length()) + "\r\n";
        for (auto [k, v] : in.header) out += k + ": " + v + "\r\n";
        out += "\r\n";
        out += in.body;

        return 0;
    }

    int send_request(const http_request& msg, int socket, int timeout, unsigned int flags) {
        using namespace flags;
        
        std::string message;
        http_write_message(msg, message, flags);

        DPRINTF("=== send http\n%s\n===\n", message.c_str());
        send(socket, message.c_str(), message.length(), 0);

        return 0;
    }

    int send_request(const http_request& msg, int socket, int timeout, sockaddr* to, size_t tsiz, unsigned int flags) {
        using namespace flags;

        std::string message;
        http_write_message(msg, message, flags);

        DPRINTF("=== send http\n%s\n===\n", message.c_str());
        if (sendto(socket, message.c_str(), message.length(), 0, to, tsiz) == -1) {
            return -1;
        };

        return 0;
    }

    int recv_response(http_response& to, int socket, int timeout) {
        FILE*       sock    = fdopen(socket, "r");
        std::string message;
        
        char mbuf[127];

        memset(mbuf, 0, 127);
        recv(socket, mbuf, 5, 0);

        if (strcmp(mbuf, "HTTP/") == 0) { // response
            to.version_major = 0;
            to.version_minor = 0;
            
            while (1) {
                recv(socket, mbuf, 1, 0);
                if (mbuf[0] == '.') break;
                to.version_major *= 10;
                to.version_major += mbuf[0] - '0';
            }

            while (1) {
                recv(socket, mbuf, 1, 0);
                if (mbuf[0] == ' ') break;
                to.version_minor *= 10;
                to.version_minor += mbuf[0] - '0';
            }

            recv(socket, mbuf, 4, 0);
            to.status = (mbuf[0] - '0') * 100 + (mbuf[1] - '0') * 10 + (mbuf[2] - '0');

            while (1) {
                recv(socket, mbuf, 1, 0);
                if (mbuf[0] == '\r') {
                    recv(socket, mbuf + 1, 1, 0);
                    if (mbuf[1] == '\n') {
                        break;
                    } else {
                        to.status_reason += mbuf[0];
                        to.status_reason += mbuf[1];
                    }
                } else {
                    to.status_reason += mbuf[0];
                }
            }

            while (1) {
                std::string f;
                std::string v;

                recv(socket, mbuf, 2, MSG_PEEK);
                if (mbuf[0] == '\r') {
                    if (mbuf[1] == '\n') {
                        recv(socket, mbuf, 2, 0);
                        break;
                    }
                }

                while (1) {
                    recv(socket, mbuf, 1, 0);
                    if (mbuf[0] == ':') break;
                    f += mbuf[0];
                }

                while (1) {
                    recv(socket, mbuf, 1, MSG_PEEK);
                    if (mbuf[0] != ' ') {
                        break;
                    } else {
                        recv(socket, mbuf, 1, 0);
                    }
                }
                
                while (1) {
                    recv(socket, mbuf, 1, 0);
                    if (mbuf[0] == '\r') {
                        recv(socket, mbuf + 1, 1, MSG_PEEK);
                        if (mbuf[1] == '\n') {
                            recv(socket, mbuf, 1, 0);
                            break;
                        }
                    }

                    v += mbuf[0];
                }

                to.header[f] = v;
            }

            if ((to.status >= 100 && to.status <= 199) || to.status == 204 || to.status == 304) {
                // these dont have a body
            } else if (to.header.contains("Transfer-Encoding") && to.header["Transfer-Encoding"] == "chunked") {
                // chunked body
            } else if (to.header.contains("Content-Length")) {
                std::string&  length_s = to.header["Content-Length"];
                unsigned long length;

                if (std::from_chars(length_s.c_str(), length_s.c_str() + length_s.length(), length).ec == std::errc(0)) {
                    while (length > 0) {
                        memset(mbuf, 0, 126);
                        
                        int read;
                        if (length < 127) {
                            read = recv(socket, mbuf, length, 0);
                        } else {
                            read = recv(socket, mbuf, 126, 0);
                        }

                        if (read > 0) {
                            length -= read;
                            to.body += mbuf;
                        } else return -1;
                    }
                }
            } else if (false) {

            } else {

            }

#ifdef DEBUG
            printf("=== recv http\nHTTP/%u.%u %u %s\r\n", 
                to.version_major,
                to.version_minor,
                to.status,
                to.status_reason.c_str());

            for (auto [n, v] : to.header) {
                printf("%s: %s\r\n", n.c_str(), v.c_str());
            }

            printf("\r\n%s\n===\n", to.body.c_str());
#endif
            
            return 0;
        } else return -1;
    }

    http_url parse_url(const std::string& surl) {
        http_url url;

        if (surl.starts_with("http://")) { //a
            unsigned long i = strlen("http://");

            for (; i < surl.length() && surl[i] != ':' && surl[i] != '/'; i++) url.host += surl[i];

            if (surl[i] == ':') {
                unsigned long start = ++i;
                for (; i < surl.length() && surl[i] != '/'; i++);

                std::from_chars(surl.c_str() + start, surl.c_str() + i, url.port);
            } 

            if (surl[i] == '/') url.path = surl.substr(++i);
        }
        
        return url;
    }
}