#pragma once

#include <stdio.h>

#include "../common/util.h"

#define SSDP_MC_PORT 1900
#define SSDP_MC_ADDR "239.255.255.250"
#define SSDP_MC_HOST SSDP_MC_ADDR ":" STRINGIFY(SSDP_MC_PORT)

namespace hatred::upnp {
    // M-SEARCH
    struct msearch_m {
        // how many seconds to wait, should be from 1 - 5 (required())
        unsigned int max_wait;

        // what to search for (required)
        const char* search_target;

        // user agent
        const char* user_agent;

        // tcp port to reply to (49152 - 65535)
        unsigned short reply_to;

        // fname (required)
        const char* friendly_name;

        // uuid i think idk
        const char* uuid;
    };

    //struct msearch_u 

    // msearch
    int send_msearch_m(int socket, msearch_m&& req);
};