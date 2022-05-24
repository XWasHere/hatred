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
        std::string method = "GET";

        std::string url;
        http_url    durl;

        int status;
        std::string status_reason;
        
        std::map<std::string, std::string> header;

        std::string body;
    };

    int send_message(const http_message& msg, int socket, int timeout);
    int recv_message(      http_message& to,  int socket, int timeout);
    
    http_url parse_url(const std::string& url);
    
    int connect(int socket, const std::string& url);
}

#endif