#ifndef HTTP_SERVER3_SERVER_HPP
#define HTTP_SERVER3_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "connection.hpp"
#include "request_handler.hpp"

namespace http {
    namespace server3 {

        class server
                : private boost::noncopyable {
        public:
            explicit server(const std::string &address, const std::string &port,
                            const std::string &doc_root, std::size_t thread_pool_size);

            void run();

        private:
            void start_accept();

            void handle_accept(const boost::system::error_code &e);

            void handle_stop();

            std::size_t thread_pool_size_;

            boost::asio::io_context io_context_;

            boost::asio::signal_set signals_;

            boost::asio::ip::tcp::acceptor acceptor_;

            connection_ptr new_connection_;

            request_handler request_handler_;
        };

    }
}

#endif
