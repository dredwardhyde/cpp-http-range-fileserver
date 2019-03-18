#include "server.hpp"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace http {
    namespace server3 {

        server::server(const std::string &address, const std::string &port,
                       const std::string &doc_root, std::size_t thread_pool_size)
                : thread_pool_size_(thread_pool_size),
                  signals_(io_context_),
                  acceptor_(io_context_),
                  new_connection_(),
                  request_handler_(doc_root) {
            signals_.add(SIGINT);
            signals_.add(SIGTERM);
#if defined(SIGQUIT)
            signals_.add(SIGQUIT);
#endif
            signals_.async_wait(boost::bind(&server::handle_stop, this));

            boost::asio::ip::tcp::resolver resolver(io_context_);
            boost::asio::ip::tcp::endpoint endpoint =
                    *resolver.resolve(address, port).begin();
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen();

            start_accept();
        }

        void server::run() {
            std::vector<boost::shared_ptr<boost::thread> > threads;
            for (std::size_t i = 0; i < thread_pool_size_; ++i) {
                boost::shared_ptr<boost::thread> thread(new boost::thread(
                        boost::bind(&boost::asio::io_context::run, &io_context_)));
                threads.push_back(thread);
            }

            for (std::size_t i = 0; i < threads.size(); ++i)
                threads[i]->join();
        }

        void server::start_accept() {
            new_connection_.reset(new connection(io_context_, request_handler_));
            acceptor_.async_accept(new_connection_->socket(),
                                   boost::bind(&server::handle_accept, this,
                                               boost::asio::placeholders::error));
        }

        void server::handle_accept(const boost::system::error_code &e) {
            if (!e) {
                new_connection_->start();
            }

            start_accept();
        }

        void server::handle_stop() {
            io_context_.stop();
        }

    }
}
