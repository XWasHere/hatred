#include <string>
#include <charconv>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "./http.h"
#include "./net.h"
#include "./util.h"

namespace hatred::http {
    int send_message(const http_message& msg, int socket, int timeout) {
        http_url dest = parse_url(msg.url);

        DPRINTF("REQUEST TO \"http://%s:%i/%s\"\n", dest.host.c_str(), dest.port, dest.path.c_str());

        return 0;
    }

    int recv_message(http_message& to, int socket, int timeout) {
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