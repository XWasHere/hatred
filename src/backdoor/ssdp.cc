#include <string.h>

#include "util.h"
#include "net.h"
#include "ssdp.h"

namespace hatred::ssdp {
    int recv_message(ssdp_message& to, int socket, int timeout) {
        int chunksread = 0;

        int         rmode = 0;
        std::string fname;
        std::string fvalue;

        char msg[1024];

        while (1) {
            memset(msg, 0, 1024);
            
            int size = 0;
            if ((size = net::readto(socket, msg, 1023, timeout)) > 0) {
                chunksread++;
                //DPRINTF("=== upnp recv message (chunk %i, size %i)\n%s\n===\n", chunksread, size, msg);

                for (int i = 0; i < size; i++) {
                    if (rmode == 0) {
                        if (size - i >= strlen("HTTP/1.1 200 OK\r\n")) {
                            const int hlen = sizeof("M-SEARCH * HTTP/1.1\r\n");
                            
                            char dat[sizeof("M-SEARCH * HTTP/1.1\r\n")];
                            memset(dat, 0, sizeof(dat));

                            switch (msg[i]) {
                                case 'N': {
                                    memcpy(dat, msg + i, strlen("NOTIFY * HTTP/1.1\r\n"));

                                    if (strcmp(dat, "NOTIFY * HTTP/1.1\r\n") == 0) { 
                                        i += strlen("NOTIFY * HTTP/1.1\r\n") - 1;
                                    } else goto bad;
                                    break;
                                }
                                case 'M': {
                                    memcpy(dat, msg + i, strlen("M-SEARCH * HTTP/1.1\r\n"));

                                    if (strcmp(dat, "M-SEARCH * HTTP/1.1\r\n") == 0) { 
                                        i += strlen("M-SEARCH * HTTP/1.1\r\n") - 1;
                                    } else goto bad;
                                    break;
                                }
                                case 'H': {
                                    memcpy(dat, msg + i, strlen("HTTP/1.1 200 OK\r\n"));

                                    if (strcmp(dat, "HTTP/1.1 200 OK\r\n") == 0) { 
                                        i += strlen("HTTP/1.1 200 OK\r\n") - 1;
                                    } else goto bad;
                                    break;
                                }
                                default: goto bad;
                            }

                            rmode = 1;
                        } else goto bad;
                    } else if (rmode == 1) {
                        one: if (msg[i] == ':') {
                            rmode = 2;
                        } else fname += msg[i];
                    } else if (rmode == 2) {
                        if (msg[i] != ' ') {
                            rmode = 3;
                            goto three;
                        }
                    } else if (rmode == 3) {
                        three: if (msg[i] == '\r') {
                            if ((i + 1 < size) && (msg[i + 1] == '\n')) {
                                i += 2;

                                //DPRINTF("SSDP FIELD: %s:%s\n", fname.c_str(), fvalue.c_str());

                                to.header[fname] = fvalue;

                                fname = "";
                                fvalue = "";

                                if ((i + 1 < size) && (msg[i] == '\r') && (msg[i + 1] == '\n')) {
                                    return 0;
                                }

                                rmode = 1;

                                goto one;
                            }
                        }
                        fvalue += msg[i];
                    }
                }
            } else break;
        }

        if (chunksread) return 0;

        return -1;

        bad: return -2;
    }
};