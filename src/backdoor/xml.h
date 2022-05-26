#ifndef HATRED_XML_H
#define HATRED_XML_H

#include <string.h>

#include <string>
#include <vector>
#include <map>

namespace hatred::xml {
    struct xml_node {
        struct iterator {
            std::vector<xml_node>& nodes;

            int pos;

            iterator(std::vector<xml_node>& nodes, int pos);

            bool      operator!=(iterator& a);
            iterator* operator++();
            xml_node& operator*();
        };

        bool exists = 1;

        bool pi = 0;

        std::string name;
        
        bool leaf = 0;
        std::string value;

        std::map<std::string, std::string> attributes;

        std::vector<xml_node> children;

        xml_node& operator[](const std::string& name);
        xml_node& operator[](const char* name);
        
        xml_node& operator[](unsigned int index);
        xml_node& operator[](int index);
        
        bool operator==(const char* a);
        bool operator==(std::string& a);

        operator bool();

        iterator begin();
        iterator end();

        explicit operator int();
    };

    xml_node parse_xml(std::string& source);
}

#endif