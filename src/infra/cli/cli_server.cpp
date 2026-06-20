#include "infra/cli/cli_server.hpp"

#include "core/logger.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <memory>
#include <string>
#include <vector>
#include <unistd.h>

namespace asio = boost::asio;

NV_NS_INFRA_CLI_BEGIN
namespace {

std::string encode_response(const std::vector<std::string>& lines) {
    std::string out;
    for (const auto& line : lines) {
        out += line;
        out += '\n';
    }
    out += ".\n";
    return out;
}

}  // namespace

class CliServer::Impl : public std::enable_shared_from_this<CliServer::Impl> {
public:
    Impl(boost::asio::io_context& io, CliDispatcher dispatcher, std::string socket_path)
        : io_(io),
          acceptor_(io),
          dispatcher_(std::move(dispatcher)),
          socket_path_(std::move(socket_path)) {}

    void start() {
        ::unlink(socket_path_.c_str());
        asio::local::stream_protocol::endpoint endpoint(socket_path_);
        acceptor_.open(endpoint.protocol());
        acceptor_.bind(endpoint);
        acceptor_.listen(asio::socket_base::max_listen_connections);
        BOOSTAPP_LOG(Info, "debug cli listening on unix://" + socket_path_);
        do_accept();
    }

    void stop() {
        boost::system::error_code ec;
        acceptor_.close(ec);
        ::unlink(socket_path_.c_str());
    }

private:
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(asio::local::stream_protocol::socket socket, CliDispatcher& dispatcher)
            : socket_(std::move(socket)), dispatcher_(dispatcher) {}

        void run() {
            do_read();
        }

    private:
        void do_read() {
            auto self = shared_from_this();
            asio::async_read_until(
                socket_,
                buffer_,
                '\n',
                [this, self](const boost::system::error_code& ec, std::size_t) {
                    if (ec) {
                        return;
                    }

                    std::istream stream(&buffer_);
                    std::string line;
                    std::getline(stream, line);

                    const auto response = encode_response(dispatcher_.dispatch(std::move(line)));
                    asio::async_write(
                        socket_,
                        asio::buffer(response),
                        [this, self](const boost::system::error_code& write_ec, std::size_t) {
                            if (write_ec) {
                                return;
                            }
                            do_read();
                        });
                });
        }

        asio::local::stream_protocol::socket socket_;
        CliDispatcher& dispatcher_;
        asio::streambuf buffer_;
    };

    void do_accept() {
        acceptor_.async_accept([this, self = shared_from_this()](
                                     const boost::system::error_code& ec,
                                     asio::local::stream_protocol::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), dispatcher_)->run();
            } else if (ec != asio::error::operation_aborted) {
                BOOSTAPP_LOG(Warning, "cli accept error: " + ec.message());
            }
            if (acceptor_.is_open()) {
                do_accept();
            }
        });
    }

    asio::io_context& io_;
    asio::local::stream_protocol::acceptor acceptor_;
    CliDispatcher dispatcher_;
    std::string socket_path_;
};

CliServer::CliServer(boost::asio::io_context& io, CliContext context)
    : socket_path_(context.config ? context.config->cli_socket : std::string{}),
      dispatcher_(std::move(context)),
      impl_(std::make_shared<Impl>(io, dispatcher_, socket_path_)) {}

void CliServer::start() {
    impl_->start();
}

void CliServer::stop() {
    impl_->stop();
}

NV_NS_INFRA_CLI_END
