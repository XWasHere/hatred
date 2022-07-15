#include "http.h"
#include <string>
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
#include "./ssdp.h"

using namespace std::string_literals;

int hatred::upnp::send_msearch_m(int socket, hatred::upnp::msearch_m&& req) {
    std::map<std::string, std::string> header = {
        { "HOST", SSDP_MC_HOST },
        { "MAN",  "\"ssdp:discover\"" },
        { "MX",   std::to_string(req.max_wait) },
        { "ST",   (req.search_target ? req.search_target : "ssdp:all") }
    };

    if (req.user_agent)    header.insert({ "USER-AGENT", req.user_agent });
    if (req.reply_to)      header.insert({ "TCPPORT.UPNP.ORG", std::to_string(req.reply_to) });
    if (req.friendly_name) header.insert({ "CPFN.UPNP.ORG", req.friendly_name });
    if (req.friendly_name) header.insert({ "CPUUID.UPNP.ORG", req.uuid });

    return http::send_request(http::http_request{
        .url = "M-SEARCH * HTTP/1.1\r\n",
        .header = header
    }, socket, 0, (sockaddr*)&ssdp::mc_address, sizeof(ssdp::mc_address), http::flags::DGRAM | http::flags::VBURL);
}