#ifndef HATRED_HTTP_H
#define HATRED_HTTP_H

#include <map>
#include <string>

namespace hatred::http {
    struct http_url {
        std::string    host;
        unsigned short port;

        std::string    path;
    };

    enum struct http_status {
        CONTINUE                        = 100,
        SWITCHING_PROTOCOLS             = 101,
        OK                              = 200,
        CREATED                         = 201,
        ACCEPTED                        = 202,
        NON_AUTHORITATIVE_INFORMATION   = 203,
        NO_CONTENT                      = 204,
        RESET_CONTENT                   = 205,
        PARTIAL_CONTENT                 = 206,
        MULTIPLE_CHOICES                = 300,
        MOVED_PERMANENTLY               = 301,
        FOUND                           = 302,
        SEE_OTHER                       = 303,
        NOT_MODIFIED                    = 304,
        USE_PROXY                       = 305,
        TEMPORARY_REDIRECT              = 306,
        BAD_REQUEST                     = 400,
        UNAUTHORIZED                    = 401,
        PAYMENT_REQUIRED                = 402,
        FORBIDDEN                       = 403,
        NOT_FOUND                       = 404,
        METHOD_NOT_ALLOWED              = 405,
        NOT_ACCEPTABLE                  = 406,
        PROXY_AUTHENTICATION_REQUIRED   = 407,
        REQUEST_TIME_OUT                = 408,
        CONFLICT                        = 409,
        GONE                            = 410,
        LENGTH_REQUIRED                 = 411,
        PRECONDITION_FAILED             = 412,
        REQUEST_ENTITY_TOO_LARGE        = 413,
        REQUEST_URI_TOO_LARGE           = 414,
        UNSUPPORTED_MEDIA_TYPE          = 415,
        REQUESTED_RANGE_NOT_SATISFYABLE = 416,
        EXPECTATION_FAILED              = 417,
        INTERNAL_SERVER_ERROT           = 500,
        NOT_IMPLEMENTED                 = 501,
        BAD_GATEWAY                     = 502,
        SERVICE_UNAVAILABLE             = 503,
        GATEWAY_TIME_OUT                = 504,
        HTTP_VERSION_NOT_SUPPORTED      = 505
    };

    struct http_response {
        unsigned int version_minor;
        unsigned int version_major;

        unsigned short status;
        std::string    status_reason;

        std::map<std::string, std::string> header;

        std::string body;
    };

    struct http_request {
        std::string method = "GET";

        std::string url;
        http_url    durl;

        int         status;
        std::string status_reason;
        
        std::map<std::string, std::string> header;

        std::string body;
    };

    int send_request(const http_request& msg, int socket, int timeout);
    int recv_request(      http_request& to,  int socket, int timeout);

    int send_response(const http_response& msg, int socket, int timeout);
    int recv_response(      http_response& to,  int socket, int timeout);
    
    http_url parse_url(const std::string& url);
    
    int connect(int socket, const std::string& url);
}

#endif