#ifndef HATRED_XML_H
#define HATRED_XML_H

#include <string>
#include <vector>
#include <map>

namespace hatred::xml {
    struct xml_node {
        std::string name;
        
        bool        leaf;
        std::string value;

        std::map<std::string, std::string> properties;

        std::vector<xml_node> children;

        xml_node* parent;
    };
}

#endif