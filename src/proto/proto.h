#pragma once

#include <type_traits>
#include <vector>
#include <string>
#include <system_error>

#include "../common/util.h"

namespace hatred::proto {
    int recv_int(int sock, int& to);
    int send_int(int sock, int value);
    constexpr int size_int() { return 4; }

    int recv_string(int sock, std::string& to);
    int send_string(int sock, const std::string& value);
    constexpr int size_string(const std::string& value) { return size_int() + value.size(); }

    namespace internal {
        template<class T>
        int generic_send(int sock, const T& value) {
            if constexpr (std::is_enum_v<T>) {
                return send_int(sock, (int&)value);
            } else {
                return value.send(sock);
            }
        }

        template<class T>
        int generic_recv(int sock, T& to) {
            if constexpr (std::is_enum_v<T>) {
                return recv_int(sock, (int&)to);
            } else {
                return T::recv(sock, to);
            }
        }

        template<class T>
        constexpr int generic_size(const T& value) {
            if constexpr (std::is_enum_v<T>) {
                return size_int();
            } else if constexpr (std::is_same_v<std::string, T>) {
                return size_string(value);
            } else {
                return value.p_size();
            }
        }
    };

    // TODO(@xwashere): this wont work for ints
    template<class T, int recv(int, T&) = internal::generic_recv<T>>
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
    template<class T, int send(int, const T&) = internal::generic_send<T>>
    int send_vector(int sock, const std::vector<T>& value) {
        if (send_int(sock, value.size())) return -1;
        
        for (const T& v : value) if (send(sock, v)) return -1;

        return 0;
    }
    template<class T, int size(const T&) = internal::generic_size<T>>
    constexpr int size_vector(const std::vector<T>& value) {
        int s = size_int();

        for (const T& v : value) {
            s += size(v);
        }

        return s;
    }

    enum class hatred_errno {
        CFAIL,  // unknown
        NEXIST, // file does not exist
        NDIR,   // not a directory
        NEMPTY  // not empty
    };

    std::string hatred_strerror(hatred_errno e);
    int         send_error(int sock, std::error_code e);

    enum class hatred_op {
        CLOSE, ERROR, ACK,
        STREAM,
        ECHO, 
        EXEC, CEXIT,
        GETFINFO, GETDIR, MKDIR, RMDIR, GETFILE, PUTFILE, RMFILE, DIRDT, FILDT, FINDT
    };

    enum class hatred_ftype {
        FILE, DIR
    };

    struct hatred_hdr {
        int       length;
        hatred_op op;

        static int recv(int sock, hatred_hdr& to);
        int send(int sock);

        static int sendp(int sock, hatred_op what);
    };

    struct hatred_error {
        hatred_errno what;

        static int recv(int sock, hatred_error& to);
        int send(int sock);

        static int sendp(int sock, hatred_errno what);

        static inline constexpr int p_size() { return sizeof(hatred_errno); }
    };

    struct hatred_echo {
        std::string message;

        static int recv(int sock, hatred_echo& to);
        int send(int sock);

        static int sendp(int sock, const std::string& message);

        static inline constexpr int p_size(const std::string& message) { return size_string(message); };
    };

    struct hatred_exec {
        std::string              cmd;
        std::vector<std::string> args;

        static int recv(int sock, hatred_exec& to);
        int send(int sock);

        static int sendp(int sock, const std::string& cmd, const std::vector<std::string>& args);

        static inline constexpr int p_size(const std::string& cmd, const std::vector<std::string>& args) { return size_string(cmd) + size_vector(args); }
    };

    struct hatred_stream {
        int         fno;
        std::string data;

        static int recv(int sock, hatred_stream& to);
        int send(int sock);

        static int sendp(int sock, int fd, const std::string& data);

        static inline constexpr int p_size(const std::string& data) { return size_int() + size_string(data); }
    };

    struct hatred_finfo {
        int type;

        std::string name;

        static int recv(int sock, hatred_finfo& to);
        int send(int sock);

        static inline constexpr int p_size(const std::string& name) { return size_int() + size_string(name); }
        inline constexpr int p_size() const { return p_size(name); }
    };

    struct hatred_dir {
        std::vector<hatred_finfo> content;

        static int recv(int sock, hatred_dir& to);
        int send(int sock);

        static inline constexpr int p_size(const std::vector<hatred_finfo>& data) { return size_vector(data); }
    };

    struct hatred_getfinfo {
        std::string name;

        static int recv(int sock, hatred_getfinfo& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);

        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };

    struct hatred_getdir {
        std::string name;
        
        static int recv(int sock, hatred_getdir& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);

        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };

    struct hatred_mkdir {
        std::string name;
        
        static int recv(int sock, hatred_mkdir& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);
        
        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };

    struct hatred_rmdir {
        std::string name;
        
        static int recv(int sock, hatred_rmdir& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);
        
        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };

    struct hatred_getfile {
        std::string name;
        
        static int recv(int sock, hatred_getfile& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);
        
        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };

    struct hatred_putfile {
        std::string name;

        static int recv(int sock, hatred_putfile& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);
        
        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };

    struct hatred_rmfile {
        std::string name;
        
        static int recv(int sock, hatred_rmfile& to);
        int send(int sock);

        static int sendp(int sock, const std::string& data);
        
        static inline constexpr int p_size(const std::string& name) { return size_string(name); }
    };
}