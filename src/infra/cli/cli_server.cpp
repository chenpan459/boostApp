#include "infra/cli/cli_server.hpp"

#include "core/logger.hpp"
#include "infra/cli/telnet_util.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <utility>

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
        Session(tcp::socket socket, CliDispatcher& dispatcher, std::string client)
            : socket_(std::move(socket)),
              dispatcher_(dispatcher),
              client_(std::move(client)) {
            socket_.set_option(tcp::no_delay(true));
        }

        void run() {
            static const std::string banner =
                "\r\nnvcomm debug cli\r\n"
                "type 'help' for commands, 'exit' to quit\r\n\r\n";
            queue_write(telnet_negotiation() + banner + telnet_prompt(),
                        [self = shared_from_this()] {
                            self->attach_log_port();
                            self->do_read();
                        });
        }

        ~Session() {
            detach_log_port();
        }

    private:
        struct PendingWrite {
            std::string data;
            std::function<void()> on_done;
        };

        void attach_log_port() {
            auto self = shared_from_this();
            log_sink_id_ = NV_NS_CORE::attach_log_sink([self](const std::string& line) {
                asio::post(self->socket_.get_executor(), [self, line]() {
                    if (self->log_sink_id_ == 0) {
                        return;
                    }
                    self->write_log_line(line);
                });
            });
            BOOSTAPP_LOG(Info, "cli telnet " + client_ + " connected, attached as log port");
        }

        void detach_log_port() {
            if (log_sink_id_ == 0) {
                return;
            }
            NV_NS_CORE::detach_log_sink(log_sink_id_);
            log_sink_id_ = 0;
        }

        void close_session() {
            detach_log_port();
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }

        void write_log_line(const std::string& line) {
            std::string out = "\r\n" + line + "\r\n" + telnet_prompt() + reader_.current_line();
            queue_write(std::move(out));
        }

        static std::string pack_replies(const std::vector<std::uint8_t>& replies) {
            return std::string(replies.begin(), replies.end());
        }

        void process_input(std::string data) {
            std::string echo;

            for (std::size_t i = 0; i < data.size(); ++i) {
                const auto parsed = reader_.feed(static_cast<std::uint8_t>(data[i]));
                if (!parsed.replies.empty()) {
                    echo += pack_replies(parsed.replies);
                }
                if (parsed.line_ready) {
                    if (!echo.empty()) {
                        queue_echo(std::move(echo));
                        echo.clear();
                    }
                    if (i + 1 < data.size()) {
                        pending_input_ = data.substr(i + 1);
                    }
                    handle_line(std::move(parsed.line));
                    return;
                }
            }

            if (!echo.empty()) {
                queue_echo(std::move(echo));
            }
            continue_input();
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
                        close_session();
                        return;
                    }
                    process_input(std::string(read_buf_.data(), n));
                });
        }

        void handle_line(std::string line) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }

            if (line == "exit" || line == "quit") {
                close_session();
                return;
            }

            std::string response;
            if (!line.empty()) {
                response += encode_telnet_lines(dispatcher_.dispatch(std::move(line)));
            }
            response += telnet_prompt();
            queue_write(std::move(response), [self = shared_from_this()] { self->continue_input(); });
        }

        void queue_write(std::string data, std::function<void()> on_done = {}) {
            write_queue_.push_back(PendingWrite{std::move(data), std::move(on_done)});
            if (!write_in_progress_) {
                flush_write_queue();
            }
        }

        void queue_echo(std::string data) {
            if (data.empty()) {
                return;
            }
            echo_queue_.push_back(PendingWrite{std::move(data), {}});
            if (!write_in_progress_) {
                flush_write_queue();
            }
        }

        void flush_write_queue() {
            if (echo_queue_.empty() && write_queue_.empty()) {
                write_in_progress_ = false;
                return;
            }

            write_in_progress_ = true;
            PendingWrite pending =
                echo_queue_.empty() ? std::move(write_queue_.front()) : std::move(echo_queue_.front());
            if (echo_queue_.empty()) {
                write_queue_.pop_front();
            } else {
                echo_queue_.pop_front();
            }

            if (pending.data.empty()) {
                if (pending.on_done) {
                    pending.on_done();
                }
                flush_write_queue();
                return;
            }

            auto payload = std::make_shared<std::string>(std::move(pending.data));
            asio::async_write(
                socket_,
                asio::buffer(*payload),
                [this, self = shared_from_this(), payload, on_done = std::move(pending.on_done)](
                    const boost::system::error_code& ec, std::size_t) mutable {
                    if (ec) {
                        close_session();
                        return;
                    }
                    if (on_done) {
                        on_done();
                    }
                    flush_write_queue();
                });
        }

        tcp::socket socket_;
        CliDispatcher& dispatcher_;
        std::string client_;
        TelnetLineReader reader_;
        std::string pending_input_;
        std::array<char, 256> read_buf_{};
        NV_NS_CORE::LogSinkId log_sink_id_{0};
        std::deque<PendingWrite> echo_queue_;
        std::deque<PendingWrite> write_queue_;
        bool write_in_progress_{false};
    };

    void do_accept() {
        acceptor_.async_accept([this, self = shared_from_this()](
                                   const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) {
                std::string client = "unknown";
                boost::system::error_code endpoint_ec;
                const auto endpoint = socket.remote_endpoint(endpoint_ec);
                if (!endpoint_ec) {
                    client = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
                }
                std::make_shared<Session>(std::move(socket), dispatcher_, std::move(client))->run();
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
