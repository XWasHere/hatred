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
#define PN(N, ...) static parse_node parse_##N(int& pos, std::string& source, ##__VA_ARGS__)

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
    int        bpos = pos;

    if (pos < source.length() && source[pos] == '<' && source[pos + 1] != '/') {
        hatred::xml::xml_node nd;
        
        pos++;
        
        int bg = pos;
        int bc = 0;
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
            while (pos < source.length() && !(P(s) || source[pos] == '/' || source[pos] == '>')) {
                pos++;
                bc++;
            }
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

namespace hatred::xml {
    xml_node* xml_parse(std::string& source) {
        int pos = 0;

        xml_node* node = new xml_node(P(document));

        for (;pos < source.length(); pos++) printf("%c", source[pos]);
        
        return node;
    }
}