#ifndef HTTP_SERVER3_REPLY_HPP
#define HTTP_SERVER3_REPLY_HPP

#include <string>
#include <vector>
#include <boost/asio.hpp>
#include "header.hpp"

namespace http {
    namespace server3 {

        struct reply {
            enum status_type {
                ok = 200,
                created = 201,
                accepted = 202,
                no_content = 204,
                partial_content = 206,
                multiple_choices = 300,
                moved_permanently = 301,
                moved_temporarily = 302,
                not_modified = 304,
                bad_request = 400,
                unauthorized = 401,
                forbidden = 403,
                not_found = 404,
                precondition_failed = 412,
                requested_range_not_satisfiable = 416,
                internal_server_error = 500,
                not_implemented = 501,
                bad_gateway = 502,
                service_unavailable = 503
            } status;

            std::vector<header> headers;

            std::string content;

            std::vector<boost::asio::const_buffer> to_buffers();

            static reply stock_reply(status_type status);
        };
    }
}

#endif
