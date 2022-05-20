#ifndef HATRED_HTTP_H
#define HATRED_HTTP_H

#include <map>
#include <string>

namespace hatred::http {
    struct http_url {
        std::string    host;
        unsigned short port;

        std::string    path;
    };

    struct http_message {
        std::string url;

        http_url    durl;
    };

    int      send_message(const http_message& msg, int socket, int timeout);
    int      recv_message(      http_message& to,  int socket, int timeout);
    
    http_url parse_url(const std::string& url);
}

#endif