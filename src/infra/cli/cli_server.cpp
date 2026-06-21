#include "infra/cli/cli_server.hpp"

#include "core/logger.hpp"
#include "infra/cli/telnet_util.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

NV_NS_INFRA_CLI_BEGIN

class CliServer::Impl : public std::enable_shared_from_this<CliServer::Impl> {
public:
    Impl(boost::asio::io_context& io,
         CliDispatcher dispatcher,
         std::string listen_address,
         std::uint16_t listen_port)
        : io_(io),
          acceptor_(io),
          dispatcher_(std::move(dispatcher)),
          listen_address_(std::move(listen_address)),
          listen_port_(listen_port) {}

    void start() {
        const auto address = asio::ip::make_address(listen_address_);
        tcp::endpoint endpoint(address, listen_port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(asio::socket_base::max_listen_connections);
        BOOSTAPP_LOG(Info,
                     "debug cli telnet listening on " + listen_address_ + ":" +
                         std::to_string(listen_port_));
        do_accept();
    }

    void stop() {
        boost::system::error_code ec;
        acceptor_.close(ec);
    }

private:
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, CliDispatcher& dispatcher)
            : socket_(std::move(socket)), dispatcher_(dispatcher) {}

        void run() {
            static const std::string banner =
                "\r\nnvcomm debug cli\r\n"
                "type 'help' for commands, 'exit' to quit\r\n"
                "tip: rlwrap telnet 127.0.0.1 2323 for command history\r\n\r\n";
            write(telnet_negotiation() + banner + telnet_prompt(),
                  [self = shared_from_this()] { self->do_read(); });
        }

    private:
        static std::string pack_replies(const std::vector<std::uint8_t>& replies) {
            return std::string(replies.begin(), replies.end());
        }

        void process_input(std::string data) {
            std::vector<std::uint8_t> replies;

            for (std::size_t i = 0; i < data.size(); ++i) {
                const auto parsed = reader_.feed(static_cast<std::uint8_t>(data[i]));
                if (!parsed.replies.empty()) {
                    replies.insert(replies.end(), parsed.replies.begin(), parsed.replies.end());
                }
                if (parsed.line_ready) {
                    if (i + 1 < data.size()) {
                        pending_input_ = data.substr(i + 1);
                    }
                    handle_line(parsed.line, pack_replies(replies));
                    return;
                }
            }

            if (!replies.empty()) {
                write(pack_replies(replies), [self = shared_from_this()] { self->continue_input(); });
                return;
            }
            do_read();
        }

        void continue_input() {
            if (!pending_input_.empty()) {
                auto next = std::move(pending_input_);
                pending_input_.clear();
                process_input(std::move(next));
                return;
            }
            do_read();
        }

        void do_read() {
            auto self = shared_from_this();
            socket_.async_read_some(
                asio::buffer(read_buf_),
                [this, self](const boost::system::error_code& ec, std::size_t n) {
                    if (ec) {
                        return;
                    }
                    process_input(std::string(read_buf_.data(), n));
                });
        }

        void handle_line(std::string line, std::string pending_replies) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }

            if (line == "exit" || line == "quit") {
                if (!pending_replies.empty()) {
                    write(std::move(pending_replies), [self = shared_from_this()] {
                        boost::system::error_code ec;
                        self->socket_.shutdown(tcp::socket::shutdown_both, ec);
                        self->socket_.close(ec);
                    });
                } else {
                    boost::system::error_code ec;
                    socket_.shutdown(tcp::socket::shutdown_both, ec);
                    socket_.close(ec);
                }
                return;
            }

            std::string response = std::move(pending_replies);
            if (!line.empty()) {
                response += encode_telnet_lines(dispatcher_.dispatch(std::move(line)));
            }
            response += telnet_prompt();
            write(std::move(response), [self = shared_from_this()] { self->continue_input(); });
        }

        void write(std::string data, std::function<void()> on_done) {
            if (data.empty()) {
                if (on_done) {
                    on_done();
                }
                return;
            }
            auto payload = std::make_shared<std::string>(std::move(data));
            asio::async_write(
                socket_,
                asio::buffer(*payload),
                [payload, on_done = std::move(on_done)](const boost::system::error_code& ec, std::size_t) {
                    if (!ec && on_done) {
                        on_done();
                    }
                });
        }

        tcp::socket socket_;
        CliDispatcher& dispatcher_;
        TelnetLineReader reader_;
        std::string pending_input_;
        std::array<char, 256> read_buf_{};
    };

    void do_accept() {
        acceptor_.async_accept([this, self = shared_from_this()](
                                   const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), dispatcher_)->run();
            } else if (ec != asio::error::operation_aborted) {
                BOOSTAPP_LOG(Warning, "cli telnet accept error: " + ec.message());
            }
            if (acceptor_.is_open()) {
                do_accept();
            }
        });
    }

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    CliDispatcher dispatcher_;
    std::string listen_address_;
    std::uint16_t listen_port_;
};

CliServer::CliServer(boost::asio::io_context& io, CliContext context)
    : listen_address_(context.config ? context.config->cli_listen : std::string{"127.0.0.1"}),
      listen_port_(context.config ? context.config->cli_port : 0),
      dispatcher_(std::move(context)),
      impl_(std::make_shared<Impl>(io, dispatcher_, listen_address_, listen_port_)) {}

void CliServer::start() {
    impl_->start();
}

void CliServer::stop() {
    impl_->stop();
}

NV_NS_INFRA_CLI_END
