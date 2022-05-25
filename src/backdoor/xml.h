#ifndef HATRED_XML_H
#define HATRED_XML_H

#include <string>
#include <vector>
#include <map>

namespace hatred::xml {
    struct xml_node {
        bool        pi = 0;

        std::string name;
        
        bool        leaf = 0;
        std::string value;

        std::map<std::string, std::string> attributes;

        std::vector<xml_node> children;

        xml_node* parent = 0;
    };

    xml_node* xml_parse(std::string& source);
}

#endif