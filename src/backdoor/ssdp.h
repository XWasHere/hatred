#ifndef HATRED_SSDP_H
#define HATRED_SSDP_H

#include <map>
#include <string>

namespace hatred::ssdp {
    // whys it called a header. ssdp messages dont have a body, so wouldnt this be the body
    struct ssdp_message {
        std::string sline;

        std::map<std::string, std::string> header;
    };
};

#endif