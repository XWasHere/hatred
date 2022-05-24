#include <string>
#include <charconv>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "./http.h"
#include "./net.h"
#include "./util.h"

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

    int send_message(const http_message& msg, int socket, int timeout) {
        http_url dest = parse_url(msg.url);

        DPRINTF("REQUEST TO \"http://%s:%i/%s\"\n", dest.host.c_str(), dest.port, dest.path.c_str());

        char msgbuf[128];

        send(socket, msgbuf, 127, 0);

        return 0;
    }

    int recv_message(http_message& to, int socket, int timeout) {
        char mbuf[128];

        net::readto(socket, mbuf, 128, 100);
        
        DPRINTF("=== recv http\n%s\n===\n", mbuf);
        
        return 0;
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