#include <string>

#include "./xml.h"

struct parse_node {
    bool good = false;

    hatred::xml::xml_node xml_node;

    operator hatred::xml::xml_node&() {
        return xml_node;
    }

    operator bool() {
        return good;
    }
};

#define P(N, ...)  parse_##N(pos, source, ##__VA_ARGS__)
#define PN(N, ...) static parse_node parse_##N(uint32_t& pos, std::string& source, ##__VA_ARGS__)

PN(s) {
    parse_node node;

    while (pos < source.length()) {
        switch (source[pos]) {
            case '\r':
            case '\n':
            case '\t':
            case ' ':  node.good = 1; break;
            default:   return node;
        }
        pos++;
    }

    return node;
}

PN(pi) {
    int bpos = pos;

    if (pos + 1 < source.length() && source.substr(pos, 2) == "<?") {
        pos += 2;

        for (; pos + 1 < source.length() && source.substr(pos, 2) != "?>"; pos++);
        pos += 2;

        return { .good = 1 };
    }

    pos = bpos;
    return { .good = 0 };
}

PN(element) {
    parse_node node;
    uint32_t   bpos = pos;

    if (pos < source.length() && source[pos] == '<' && source[pos + 1] != '/') {
        hatred::xml::xml_node nd;
        
        pos++;
        
        uint32_t bg = pos;
        uint32_t bc = 0;
        while (pos < source.length() && !(P(s) || source[pos] == '/' || source[pos] == '>')) {
            bc++;
            pos++;
        }
        nd.name = source.substr(bg, bc);
        
        while (pos < source.length() && source[pos] != '>' && source[pos] != '/') {
            P(s);
            
            std::string name;
            std::string value;

            bg = pos;
            bc = 0;
            while (pos < source.length() && !(P(s) || source[pos] == '=')) {
                pos++;
                bc++;
            }
            pos++;
            name = source.substr(bg, bc);

            P(s);
            
            pos++;
            bg = pos;
            bc = 0;
            while (pos < source.length() && !(P(s) || source[pos] == '"' || source[pos] == '\"')) {
                pos++;
                bc++;
            }
            pos++;
            value = source.substr(bg, bc - 1);

            nd.attributes[name] = value;
        }

        if (pos < source.length() && source[pos] == '/') {
            pos++;
            if (pos < source.length() && source[pos] == '>') {
                pos++;
                node.xml_node = nd;
                node.good = 1;
                return node;
            } else {
                pos = bpos;
                return node;
            }
        }
        if (pos < source.length() && source[pos] == '>') pos++;

        P(s);

        bool leaf = 1;
        parse_node child;
        while (child = P(element)) {
            P(s);
            nd.children.push_back(child);
            leaf = 0;
        }

        if (leaf) {
            bg = pos;
            bc = 0;
            while (pos < source.length() && source[pos] != '<') {
                bc++;
                pos++;
            }

            nd.leaf = 1;
            nd.value = source.substr(bg, bc);
        }

        P(s);

        if (pos < source.length() && source[pos] == '<') {
            pos++;
            if (pos < source.length() && source[pos] == '/') {
                pos++;
                if (pos + nd.name.length() < source.length() && source.substr(pos, nd.name.length()) == nd.name) {
                    pos += nd.name.length();
                    if (pos < source.length() && source[pos] == '>') {
                        pos++;

                        node.xml_node = nd;
                        node.good = 1;
                        return node;
                    }
                }
            }
        }
    }

    pos = bpos;
    return node;
}

PN(document) {
    parse_node root;

    P(s);
    while (P(pi)) P(s);
    P(s);

    root = P(element);

    return root;
}

static hatred::xml::xml_node null_node { .exists = 0 };

namespace hatred::xml {
    xml_node parse_xml(std::string& source) {
        uint32_t pos = 0;

        return P(document);
    }

    xml_node& xml_node::operator[](const std::string& name) {
        for (xml_node& i : children) 
            if (i.name == name)
                return i;

        return null_node;
    }

    xml_node& xml_node::operator[](const char* name) { 
        return operator[](std::string(name));
    }

    xml_node& xml_node::operator[](unsigned int index) {
        if (index + 1 > children.size()) return null_node;

        return children[index];
    }

    xml_node& xml_node::operator[](int index) {
        return operator[]((unsigned int)index);
    }

    bool xml_node::operator==(const char* a) {
        return strcmp(value.c_str(), a) == 0;
    }

    bool xml_node::operator==(std::string& a) {
        return value == a;
    }

    xml_node::operator bool() {
        return exists;
    }

    xml_node::iterator xml_node::begin() {
        return xml_node::iterator(children, 0);
    }

    xml_node::iterator xml_node::end() {
        return xml_node::iterator(children, children.size());
    }

    xml_node::iterator::iterator(std::vector<xml_node>& nodes, int pos) : nodes(nodes), pos(pos) {}

    bool xml_node::iterator::operator!=(xml_node::iterator& a) {
        return pos != a.pos;
    }

    xml_node::iterator* xml_node::iterator::operator++() {
        pos++;
        return this;
    }

    xml_node& xml_node::iterator::operator*() {
        return nodes[pos];
    }
}