#ifndef HTTP_SERVER3_CONNECTION_HPP
#define HTTP_SERVER3_CONNECTION_HPP

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"

namespace http {
    namespace server3 {

        class connection
                : public boost::enable_shared_from_this<connection>,
                  private boost::noncopyable {
        public:
            explicit connection(boost::asio::io_context &io_context,
                                request_handler &handler);

            boost::asio::ip::tcp::socket &socket();

            void start();

        private:
            void handle_read(const boost::system::error_code &e,
                             std::size_t bytes_transferred);

            void handle_write(const boost::system::error_code &e);

            boost::asio::io_context::strand strand_;

            boost::asio::ip::tcp::socket socket_;

            request_handler &request_handler_;

            boost::array<char, 8192> buffer_;

            request request_;

            request_parser request_parser_;

            reply reply_;
        };

        typedef boost::shared_ptr<connection> connection_ptr;

    }
}

#endif
