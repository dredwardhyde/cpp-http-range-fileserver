#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "request_handler.hpp"

namespace http {
    namespace server3 {

        connection::connection(boost::asio::io_context &io_context,
                               request_handler &handler)
                : strand_(io_context),
                  socket_(io_context),
                  request_handler_(handler) {
        }

        boost::asio::ip::tcp::socket &connection::socket() {
            return socket_;
        }

        void connection::start() {
            socket_.async_read_some(boost::asio::buffer(buffer_),
                                    boost::asio::bind_executor(strand_,
                                                               boost::bind(&connection::handle_read, shared_from_this(),
                                                                           boost::asio::placeholders::error,
                                                                           boost::asio::placeholders::bytes_transferred)));
        }

        void connection::handle_read(const boost::system::error_code &e,
                                     std::size_t bytes_transferred) {
            if (!e) {
                boost::tribool result;
                boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
                        request_, buffer_.data(), buffer_.data() + bytes_transferred);

                if (result) {
                    request_handler_.handle_request(request_, reply_);
                    boost::asio::async_write(socket_, reply_.to_buffers(),
                                             boost::asio::bind_executor(strand_,
                                                                        boost::bind(&connection::handle_write,
                                                                                    shared_from_this(),
                                                                                    boost::asio::placeholders::error)));
                } else if (!result) {
                    reply_ = reply::stock_reply(reply::bad_request);
                    boost::asio::async_write(socket_, reply_.to_buffers(),
                                             boost::asio::bind_executor(strand_,
                                                                        boost::bind(&connection::handle_write,
                                                                                    shared_from_this(),
                                                                                    boost::asio::placeholders::error)));
                } else {
                    socket_.async_read_some(boost::asio::buffer(buffer_),
                                            boost::asio::bind_executor(strand_,
                                                                       boost::bind(&connection::handle_read,
                                                                                   shared_from_this(),
                                                                                   boost::asio::placeholders::error,
                                                                                   boost::asio::placeholders::bytes_transferred)));
                }
            }

        }

        void connection::handle_write(const boost::system::error_code &e) {
            if (!e) {
                boost::system::error_code ignored_ec;
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            }
        }
    }
}
