#pragma once

#include <map>
#include <string>

#ifndef __WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#define SSDP_MC_PORT 1900
#define SSDP_MC_ADDR "239.255.255.250"
#define SSDP_MC_HOST SSDP_MC_ADDR ":" STRINGIFY(SSDP_MC_PORT)

namespace hatred::ssdp {
    static const sockaddr_in mc_address = {
        .sin_family = AF_INET,
        .sin_port   = htons(SSDP_MC_PORT),
#ifndef _WIN32
        .sin_addr   = { .s_addr = inet_addr(SSDP_MC_ADDR) }
#else
        .sin_addr   = { .S_un = { .S_addr = inet_addr(SSDP_MC_ADDR) } }
#endif
    };

    // whys it called a header. ssdp messages dont have a body, so wouldnt this be the body
    struct ssdp_message {
        std::string sline;

        std::map<std::string, std::string> header;
    };

    int recv_message(ssdp_message& to, int socket, int timeout);
};