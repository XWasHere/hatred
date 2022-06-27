#ifndef __WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include "./upnp.h"

using namespace std::string_literals;

static sockaddr_in ssdp_mc_address = {
    .sin_family = AF_INET,
    .sin_port   = htons(SSDP_MC_PORT),
#ifndef _WIN32
    .sin_addr   = { .s_addr = inet_addr(SSDP_MC_ADDR) }
#else
    .sin_addr   = { .S_un = { .S_addr = inet_addr(SSDP_MC_ADDR) } }
#endif
};

int hatred::upnp::send_msearch_m(int socket, hatred::upnp::msearch_m&& req) {
    std::stringstream packet_build;
    
    packet_build << "M-SEARCH * HTTP/1.1\r\n"
                    "HOST: " SSDP_MC_HOST "\r\n"
                    "MAN: \"ssdp:discover\"\r\n"
                    "MX: " << req.max_wait << "\r\n"
                    "ST: " << (req.search_target ? req.search_target : "ssdp:all") << "\r\n";

    if (req.user_agent)    packet_build << "USER-AGENT: " << req.user_agent << "\r\n";
    if (req.reply_to)      packet_build << "TCPPORT.UPNP.ORG: " << req.reply_to << "\r\n";
    if (req.friendly_name) packet_build << "CPFN.UPNP.ORG: " << req.friendly_name << "\r\n";
    if (req.friendly_name) packet_build << "CPUUID.UPNP.ORG: " << req.uuid << "\r\n";

    std::string packet = packet_build.str();
    
    DPRINTF("=== upnp send mc msearch\n%s\n===\n", packet.c_str());

    if (sendto(socket, packet.c_str(), packet.length(), 0, (sockaddr*)&ssdp_mc_address, sizeof(ssdp_mc_address)) == -1) {
        return -1;
    };

    return 0;
}