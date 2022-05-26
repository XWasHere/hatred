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

    int recv_message(http_message& to, int socket, int timeout) {
        std::string message;

        char mbuf[127];

        while (net::readto(socket, mbuf, 126, timeout)) {
            message += mbuf;
            memset(mbuf, 0, 126);
        }
        
        DPRINTF("=== recv http\n%s\n===\n", message.c_str());
        
        int size = message.length();
        int pos  = 0;

        if (size < strlen("HTTP/1.1 ")) {
            return -1;
        }

        if (message.substr(0, strlen("HTTP/1.1 ")) != "HTTP/1.1 ") {
            return -1;
        }

        pos += strlen("HTTP/1.1 ");
        
        int start = pos;
        while (message[pos++] != ' ');
        std::from_chars(message.c_str() + start, message.c_str() + pos, to.status);

        start = pos;
        while (message[pos++] != '\r' || message[pos] != '\n');
        to.status_reason = message.substr(start, pos - start);
        pos++;
        
        while (message[pos] != '\r' || message[pos + 1] != '\n') {
            std::string key;
            std::string value;

            for (start = pos; message[pos++] != ':';);
            key = message.substr(start, pos - start - 1);

            for (start = pos; message[pos] == ' '; pos++);

            for (start = pos; message[pos++] != '\r' || message[pos] != '\n';);
            value = message.substr(start, pos - start - 1);
            pos++;

            to.header[key] = value;
        }

        pos += 2;

        to.body = message.substr(pos);

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