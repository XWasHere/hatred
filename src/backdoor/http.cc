#include <string>
#include <charconv>

#include <sys/socket.h>
#include <arpa/inet.h>
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
            .sin_addr   = { .s_addr = inet_addr(to.host.c_str()) }
        };

        if (::connect(socket, (sockaddr*)&addr, sizeof(addr))) {
            DPRINTF("connection issue, please implement error information niko\n");
            return 1;
        }

        return 0;
    }

    int send_request(const http_request& msg, int socket, int timeout) {
        http_url dest = parse_url(msg.url);
        std::string message;

        DPRINTF("request to \"http://%s:%i/%s\"\n", dest.host.c_str(), dest.port, dest.path.c_str());

        message += msg.method + " /" + dest.path + " HTTP/1.1\r\n";
        if (!msg.header.contains("Host")) message += "Host: " + dest.host + ":" + std::to_string(dest.port) + "\r\n";
        if (!msg.header.contains("Content-Length")) message += "Content-Length: " + std::to_string(msg.body.length()) + "\r\n";
        for (auto [k, v] : msg.header) message += k + ": " + v + "\r\n";
        message += "\r\n";
        message += msg.body;

        DPRINTF("=== send http\n%s\n===\n", message.c_str());
        send(socket, message.c_str(), message.length(), 0);

        return 0;
    }

    int recv_response(http_response& to, int socket, int timeout) {
        FILE*       sock    = fdopen(socket, "r");
        std::string message;
        
        char mbuf[127];

        memset(mbuf, 0, 127);
        fread(mbuf, 1, 5, sock);
        
        if (strcmp(mbuf, "HTTP/") == 0) { // response
            to.version_major = 0;
            to.version_minor = 0;
            
            while (1) {
                fread(mbuf, 1, 1, sock);
                if (mbuf[0] == '.') break;
                to.version_major *= 10;
                to.version_major += mbuf[0] - '0';
            }

            while (1) {
                fread(mbuf, 1, 1, sock);
                if (mbuf[0] == ' ') break;
                to.version_minor *= 10;
                to.version_minor += mbuf[0] - '0';
            }

            fread(mbuf, 1, 4, sock);
            to.status = (mbuf[0] - '0') * 100 + (mbuf[1] - '0') * 10 + (mbuf[2] - '0');

            while (1) {
                fread(mbuf, 1, 1, sock);
                if (mbuf[0] == '\r') {
                    fread(mbuf + 1, 1, 1, sock);
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

                fread(mbuf, 1, 1, sock);
                if (mbuf[0] == '\r') {
                    fread(mbuf + 1, 1, 1, sock);
                    if (mbuf[1] == '\n') {
                        break;
                    }
                    ungetc(mbuf[1], sock);
                }
                ungetc(mbuf[0], sock);

                while (1) {
                    fread(mbuf, 1, 1, sock);
                    if (mbuf[0] == ':') break;
                    f += mbuf[0];
                }

                while (1) {
                    fread(mbuf, 1, 1, sock);
                    if (mbuf[0] != ' ') {
                        ungetc(mbuf[0], sock);
                        break;
                    }
                }
                
                while (1) {
                    fread(mbuf, 1, 1, sock);
                    if (mbuf[0] == '\r') {
                        fread(mbuf + 1, 1, 1, sock);
                        if (mbuf[1] == '\n') {
                            break;
                        }
                        ungetc(mbuf[1], sock);
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
                            read = fread(mbuf, 1, length, sock);
                        } else {
                            read = fread(mbuf, 1, 126, sock);
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

        if (surl.length() >= strlen("http://") && surl.substr(0, strlen("http://")) == "http://") {
            int i = strlen("http://");

            for (; i < surl.length() && surl[i] != ':' && surl[i] != '/'; i++) url.host += surl[i];

            if (surl[i] == ':') {
                int start = ++i;
                for (; i < surl.length() && surl[i] != '/'; i++);

                std::from_chars(surl.c_str() + start, surl.c_str() + i, url.port);
            } 

            if (surl[i] == '/') url.path = surl.substr(++i);
        }
        
        return url;
    }
}