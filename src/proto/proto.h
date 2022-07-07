#pragma once

#include <vector>
#include <string>

namespace hatred::proto {
    int recv_int(int sock, int& to);
    int send_int(int sock, int value);
    
    int recv_string(int sock, std::string& to);
    int send_string(int sock, const std::string& value);

    // TODO(@xwashere): this wont work for ints
    template<class T, int recv(int, T&)>
    int recv_vector(int sock, std::vector<T>& to) {
        int length;
        if (recv_int(sock, length)) return -1;

        if (to.capacity() < length) to.resize(length);
        
        for (int i = 0; i < length; i++) {
            T val;
            if (recv(sock, val)) return -1;
            to[i] = val;
        }
        
        return 0;
    }
    template<class T, int send(int, const T&)>
    int send_vector(int sock, const std::vector<T>& value) {
        if (send_int(sock, value.size())) return -1;
        
        for (const T& v : value) if (send(sock, v)) return -1;

        return 0;
    }

    enum class hatred_errno {

    };

    enum class hatred_op {
        CLOSE, ERROR,
        ECHO, 
        EXEC, STREAM,
        GETFINFO, GETDIR, MKDIR, RMDIR, GETFILE, PUTFILE, RMFILE
    };

    struct hatred_hdr {
        int length;
        int op;

        static int recv(int sock, hatred_hdr& to);
        int send(int sock);
    };

    struct hatred_error {
        hatred_errno what;
    };

    struct hatred_echo {
        std::string message;

        static int recv(int sock, hatred_echo& to);
        int send(int sock);
    };

    struct hatred_exec {
        std::string              cmd;
        std::vector<std::string> args;

        static int recv(int sock, hatred_exec& to);
        int send(int sock);
    };

    struct hatred_stream {
        int         fno;
        std::string data;

        static int recv(int sock, hatred_stream& to);
        int send(int sock);
    };

    struct hatred_finfo {
        bool dir;

        std::string name;

        static int recv(int sock, hatred_finfo& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_dir {
        std::vector<hatred_finfo> content;

        static int recv(int sock, hatred_dir& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_file {
        std::string content;

        static int recv(int sock, hatred_file& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_getfinfo {
        std::string name;

        static int recv(int sock, hatred_getfinfo& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_getdir {
        std::string name;
        
        static int recv(int sock, hatred_getdir& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_mkdir {
        std::string name;
        
        static int recv(int sock, hatred_mkdir& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_rmdir {
        std::string name;
        
        static int recv(int sock, hatred_rmdir& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_getfile {
        std::string name;
        
        static int recv(int sock, hatred_getfile& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_putfile {
        std::string name;
        hatred_file file;

        static int recv(int sock, hatred_putfile& to) = delete;
        int send(int sock) = delete;
    };

    struct hatred_rmfile {
        std::string name;
        
        static int recv(int sock, hatred_rmfile& to) = delete;
        int send(int sock) = delete;
    };
}