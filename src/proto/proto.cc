#include <sys/types.h>
#include <system_error>
#ifndef __WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <string.h>
#include <errno.h>

#include "./proto.h"

#ifdef DEBUG
#ifndef __WIN32
#define DDIE(x) { perror(x); return -1; }
#else
#define DDIE(x) { printf(x ":%i\n", WSAGetLastError()); return -1; }
#endif
#else
#define DDIE(x) { return -1; }
#endif

namespace hatred::proto {
    std::string hatred_strerror(hatred_errno e) {
        switch (e) {
            case hatred_errno::NEXIST: return "file does not exist";
            case hatred_errno::NDIR:   return "not a directory";
            case hatred_errno::NEMPTY: return "not empty";
            case hatred_errno::CFAIL:
            default:                   return "catastrophic failure";
        }
    }

    int send_error(int sock, std::error_code e) {
        if (hatred_hdr{.length = 0, .op = hatred_op::ERROR}.send(sock)) return -1;
        
        hatred_errno en = hatred_errno::CFAIL;

        if      (e == std::errc::no_such_file_or_directory) en = hatred_errno::NEXIST;
        else if (e == std::errc::not_a_directory)           en = hatred_errno::NDIR;
        else if (e == std::errc::directory_not_empty)       en = hatred_errno::NEMPTY;

        if (hatred_error{.what = en}.send(sock)) return -1;

        return 0;
    }

    int recv_string(int sock, std::string& to) {
        int length;
        char* value;

        if (recv_int(sock, length)) return -1;

        if (length == 0) {
            to = std::string("");
            return 0;
        } else if (length < 128) {
            value = (char*)alloca(length + 1);
            memset(value, 0, length + 1);
            if (recv(sock, (char*)value, length, 0) <= 0) return -1;
            to = std::string(value);
            return 0;
        } else {
            if ((value = (char*)malloc(length + 1)) == 0) return -1;
            memset(value, 0, length + 1);
            
            int left = length; // this used to just read one big bit but recently i learned that sometimes that doesnt totally work
            int pos  = 0;
            ssize_t res;
            while ((res = recv(sock, (char*)value + pos, left, 0)) && res > 0) {
                pos += res;
                left -= res;
            }
            if (res == -1) return -1;

            to = std::string(value, length);
            free(value);
            return 0;
        }
    }
    
    int send_string(int sock, const std::string& value) {
        if (send_int(sock, value.length()))                     return -1;
        if (send(sock, value.c_str(), value.length(), 0) == -1) return -1;

        return 0;
    }

    int recv_int(int sock, int& to) {
        int dat;
        if (recv(sock, (char*)&dat, 4, 0) <= 0) DDIE("recv");
        to = ntohl(dat);
        return 0;
    }

    int send_int(int sock, int value) {
        int dat = htonl(value);
        if (send(sock, (char*)&dat, 4, 0) < 0) DDIE("send");
        return 0;
    }

    int hatred_hdr::recv(int sock, hatred_hdr& to) {
        if (recv_int(sock, to.length))           return -1;
        if (internal::generic_recv(sock, to.op)) return -1;

        return 0;
    }

    int hatred_hdr::send(int sock) const {
        if (send_int(sock, length))           return -1;
        if (internal::generic_send(sock, op)) return -1;

        return 0;
    }

    int hatred_hdr::sendp(int sock, hatred_op op) {
        return hatred_hdr{.length = 0, .op=op}.send(sock);
    }

    int hatred_error::recv(int sock, hatred_error &to) {
        if (internal::generic_recv(sock, to.what)) return -1;

        return 0;
    }

    int hatred_error::send(int sock) const {
        if (internal::generic_send(sock, what)) return -1;

        return 0;
    }

    int hatred_error::sendp(int sock, hatred_errno what) {
        if (hatred_hdr{
            .length = hatred_error::p_size(),
            .op     = hatred_op::ERROR
        }.send(sock)) return -1;

        if (hatred_error{
            .what = what
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_stream::recv(int sock, hatred_stream& to) {
        if (recv_int(sock, to.fno)) return -1;
        if (recv_string(sock, to.data)) return -1;

        return 0;
    }

    int hatred_stream::send(int sock) const {
        if (send_int(sock, fno)) return -1;
        if (send_string(sock, data)) return -1;

        return 0;
    }

    int hatred_stream::sendp(int sock, int fd, const std::string &data) {
        if (hatred_hdr{
            .length = p_size(data),
            .op     = hatred_op::STREAM
        }.send(sock)) return -1;

        if (hatred_stream{
            .fno  = fd,
            .data = data
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_echo::recv(int sock, hatred_echo& to) {
        if (recv_string(sock, to.message)) return -1;

        return 0;
    }

    int hatred_echo::send(int sock) const {
        if (send_string(sock, message)) return -1;

        return 0;
    }

    int hatred_echo::sendp(int sock, const std::string &message) {
        if (hatred_hdr{
            .length = hatred_echo::p_size(message),
            .op     = hatred_op::ECHO
        }.send(sock)) return -1;

        if (hatred_echo{
            .message = message
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_exec::recv(int sock, hatred_exec& to) {
        if (recv_string(sock, to.cmd)) return -1;
        if (recv_vector<std::string, recv_string>(sock, to.args)) return -1;

        return 0;
    }

    int hatred_exec::send(int sock) const {
        if (send_string(sock, cmd)) return -1;
        if (send_vector<std::string, send_string>(sock, args)) return -1;

        return 0;
    }

    int hatred_exec::sendp(int sock, const std::string &cmd, const std::vector<std::string> &args) {
        if (hatred_hdr{
            .length = hatred_exec::p_size(cmd, args),
            .op     = hatred_op::EXEC
        }.send(sock)) return -1;

        if (hatred_exec{
            .cmd  = cmd,
            .args = args
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_finfo::recv(int sock, hatred_finfo &to) {
        if (recv_int(sock, to.type))    return -1;
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_finfo::send(int sock) const {
        if (send_int(sock, type))    return -1;
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_dir::recv(int sock, hatred_dir &to) {
        if (recv_vector<hatred_finfo>(sock, to.content)) return -1;

        return 0;
    }

    int hatred_dir::send(int sock) const {
        if (send_vector<hatred_finfo>(sock, content)) return -1;

        return 0;
    }

    int hatred_getfinfo::recv(int sock, hatred_getfinfo &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_getfinfo::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_getfinfo::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_getfinfo::p_size(name),
            .op     = hatred_op::GETFINFO
        }.send(sock)) return -1;

        if (hatred_getfinfo{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_getdir::recv(int sock, hatred_getdir &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_getdir::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_getdir::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_getdir::p_size(name),
            .op     = hatred_op::GETDIR
        }.send(sock)) return -1;

        if (hatred_getdir{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_mkdir::recv(int sock, hatred_mkdir &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_mkdir::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_mkdir::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_mkdir::p_size(name),
            .op     = hatred_op::GETDIR
        }.send(sock)) return -1;

        if (hatred_mkdir{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_rmdir::recv(int sock, hatred_rmdir &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_rmdir::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_rmdir::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_rmdir::p_size(name),
            .op     = hatred_op::RMDIR
        }.send(sock)) return -1;

        if (hatred_rmdir{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_getfile::recv(int sock, hatred_getfile &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_getfile::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_getfile::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_getfile::p_size(name),
            .op     = hatred_op::GETFILE
        }.send(sock)) return -1;

        if (hatred_getfile{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_putfile::recv(int sock, hatred_putfile &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_putfile::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_putfile::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_putfile::p_size(name),
            .op     = hatred_op::PUTFILE
        }.send(sock)) return -1;

        if (hatred_putfile{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }

    int hatred_rmfile::recv(int sock, hatred_rmfile &to) {
        if (recv_string(sock, to.name)) return -1;

        return 0;
    }

    int hatred_rmfile::send(int sock) const {
        if (send_string(sock, name)) return -1;

        return 0;
    }

    int hatred_rmfile::sendp(int sock, const std::string &name) {
        if (hatred_hdr{
            .length = hatred_rmfile::p_size(name),
            .op     = hatred_op::RMFILE
        }.send(sock)) return -1;

        if (hatred_rmfile{
            .name = name
        }.send(sock)) return -1;

        return 0;
    }
}