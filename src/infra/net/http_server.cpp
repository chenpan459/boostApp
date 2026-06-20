#include "infra/net/http_server.hpp"

#include "core/logger.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <memory>
#include <string>
#include <unistd.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

NV_NS_INFRA_NET_BEGIN
namespace {

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    using Stream = beast::tcp_stream;

    HttpSession(Stream stream,
                service::PacketProcessor& processor,
                std::size_t max_body_bytes)
        : stream_(std::move(stream)),
          processor_(processor),
          max_body_bytes_(max_body_bytes) {}

    void run() {
        do_read();
    }

private:
    void do_read() {
        request_ = {};
        stream_.expires_after(std::chrono::seconds(30));
        http::async_read(
            stream_,
            buffer_,
            request_,
            beast::bind_front_handler(&HttpSession::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t) {
        if (ec == http::error::end_of_stream) {
            return;
        }
        if (ec) {
            return;
        }

        if (request_.method() != http::verb::get && request_.method() != http::verb::post) {
            send_response({405, "application/json", R"({"error":"method not allowed"})"});
            return;
        }

        domain::NetworkPacket packet;
        packet.path = std::string(request_.target());
        packet.body = request_.body();
        packet.client = remote_endpoint();

        if (packet.path == "/health") {
            send_response(processor_.process_sync(packet));
            return;
        }

        if (request_.method() != http::verb::post) {
            send_response({405, "application/json", R"({"error":"POST required"})"});
            return;
        }

        if (packet.body.size() > max_body_bytes_) {
            send_response({413, "application/json", R"({"error":"payload too large"})"});
            return;
        }

        auto self = shared_from_this();
        processor_.process_async(std::move(packet), [self](domain::PacketResult result) {
            asio::post(self->stream_.get_executor(), [self, result = std::move(result)]() mutable {
                self->send_response(std::move(result));
            });
        });
    }

    void send_response(domain::PacketResult result) {
        response_ = {};
        response_.version(request_.version());
        response_.result(static_cast<http::status>(result.status_code));
        response_.set(http::field::server, "nvcommd");
        response_.set(http::field::content_type, result.content_type);
        response_.keep_alive(request_.keep_alive());
        response_.body() = std::move(result.body);
        response_.prepare_payload();

        stream_.expires_after(std::chrono::seconds(30));
        http::async_write(
            stream_,
            response_,
            beast::bind_front_handler(&HttpSession::on_write, shared_from_this(), response_.need_eof()));
    }

    void on_write(bool close, beast::error_code ec, std::size_t) {
        if (ec) {
            return;
        }
        if (close) {
            beast::error_code ignored;
            stream_.socket().shutdown(tcp::socket::shutdown_send, ignored);
            return;
        }
        do_read();
    }

    std::string remote_endpoint() const {
        beast::error_code ec;
        const auto endpoint = stream_.socket().remote_endpoint(ec);
        if (ec) {
            return "unknown";
        }
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    Stream stream_;
    service::PacketProcessor& processor_;
    std::size_t max_body_bytes_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
};

class LocalHttpSession : public std::enable_shared_from_this<LocalHttpSession> {
public:
    using UnixStream = beast::basic_stream<asio::local::stream_protocol>;

    LocalHttpSession(asio::local::stream_protocol::socket socket,
                     service::PacketProcessor& processor,
                     std::size_t max_body_bytes)
        : stream_(std::move(socket)),
          processor_(processor),
          max_body_bytes_(max_body_bytes) {}

    void run() {
        do_read();
    }

private:
    void do_read() {
        request_ = {};
        stream_.expires_after(std::chrono::seconds(30));
        http::async_read(
            stream_,
            buffer_,
            request_,
            beast::bind_front_handler(&LocalHttpSession::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t) {
        if (ec == http::error::end_of_stream) {
            return;
        }
        if (ec) {
            return;
        }

        domain::NetworkPacket packet;
        packet.path = std::string(request_.target());
        packet.body = request_.body();
        packet.client = "unix";

        if (packet.path == "/health") {
            send_response(processor_.process_sync(packet));
            return;
        }

        if (request_.method() != http::verb::post) {
            send_response({405, "application/json", R"({"error":"POST required"})"});
            return;
        }

        if (packet.body.size() > max_body_bytes_) {
            send_response({413, "application/json", R"({"error":"payload too large"})"});
            return;
        }

        auto self = shared_from_this();
        processor_.process_async(std::move(packet), [self](domain::PacketResult result) {
            asio::post(self->stream_.get_executor(), [self, result = std::move(result)]() mutable {
                self->send_response(std::move(result));
            });
        });
    }

    void send_response(domain::PacketResult result) {
        response_ = {};
        response_.version(request_.version());
        response_.result(static_cast<http::status>(result.status_code));
        response_.set(http::field::server, "nvcommd");
        response_.set(http::field::content_type, result.content_type);
        response_.keep_alive(request_.keep_alive());
        response_.body() = std::move(result.body);
        response_.prepare_payload();

        http::async_write(
            stream_,
            response_,
            beast::bind_front_handler(&LocalHttpSession::on_write, shared_from_this(), response_.need_eof()));
    }

    void on_write(bool close, beast::error_code ec, std::size_t) {
        if (ec) {
            return;
        }
        if (close) {
            return;
        }
        do_read();
    }

    UnixStream stream_;
    service::PacketProcessor& processor_;
    std::size_t max_body_bytes_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
};

}  // namespace

class HttpServer::TcpListener : public std::enable_shared_from_this<HttpServer::TcpListener> {
public:
    TcpListener(asio::io_context& io,
                tcp::endpoint endpoint,
                service::PacketProcessor& processor,
                std::size_t max_body_bytes)
        : io_(io),
          acceptor_(io),
          processor_(processor),
          max_body_bytes_(max_body_bytes) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            throw std::runtime_error("tcp open failed: " + ec.message());
        }
        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        acceptor_.bind(endpoint, ec);
        if (ec) {
            throw std::runtime_error("tcp bind failed: " + ec.message());
        }
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            throw std::runtime_error("tcp listen failed: " + ec.message());
        }
    }

    void run() {
        do_accept();
    }

    void stop() {
        beast::error_code ec;
        acceptor_.close(ec);
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            asio::make_strand(io_),
            beast::bind_front_handler(&TcpListener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<HttpSession>(beast::tcp_stream(std::move(socket)), processor_, max_body_bytes_)
                ->run();
        }
        do_accept();
    }

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    service::PacketProcessor& processor_;
    std::size_t max_body_bytes_;
};

class HttpServer::UnixListener : public std::enable_shared_from_this<HttpServer::UnixListener> {
public:
    UnixListener(asio::io_context& io,
                 const std::string& path,
                 service::PacketProcessor& processor,
                 std::size_t max_body_bytes)
        : io_(io),
          acceptor_(io),
          processor_(processor),
          max_body_bytes_(max_body_bytes) {
        ::unlink(path.c_str());
        asio::local::stream_protocol::endpoint endpoint(path);
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        acceptor_.bind(endpoint, ec);
        if (ec) {
            throw std::runtime_error("unix bind failed: " + ec.message());
        }
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            throw std::runtime_error("unix listen failed: " + ec.message());
        }
    }

    void run() {
        do_accept();
    }

    void stop() {
        beast::error_code ec;
        acceptor_.close(ec);
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            asio::make_strand(io_),
            beast::bind_front_handler(&UnixListener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, asio::local::stream_protocol::socket socket) {
        if (!ec) {
            std::make_shared<LocalHttpSession>(std::move(socket), processor_, max_body_bytes_)->run();
        }
        do_accept();
    }

    asio::io_context& io_;
    asio::local::stream_protocol::acceptor acceptor_;
    service::PacketProcessor& processor_;
    std::size_t max_body_bytes_;
};

HttpServer::HttpServer(const core::AppConfig& config,
                       IoContextPool& pool,
                       service::PacketProcessor& processor)
    : config_(config), pool_(pool), processor_(processor) {}

void HttpServer::start() {
    const auto max_body_bytes = static_cast<std::size_t>(config_.max_body_kb) * 1024;
    auto& io = pool_.io_context();

    const auto address = asio::ip::make_address(config_.listen_address);
    tcp::endpoint endpoint(address, config_.listen_port);
    tcp_listener_ = std::make_shared<TcpListener>(io, endpoint, processor_, max_body_bytes);
    tcp_listener_->run();
    BOOSTAPP_LOG(Info, "http listening on " + config_.listen_address + ":" + std::to_string(config_.listen_port));

    if (!config_.unix_socket.empty()) {
        unix_listener_ = std::make_shared<UnixListener>(io, config_.unix_socket, processor_, max_body_bytes);
        unix_listener_->run();
        BOOSTAPP_LOG(Info, "http listening on unix://" + config_.unix_socket);
    }
}

void HttpServer::stop() {
    if (tcp_listener_) {
        tcp_listener_->stop();
    }
    if (unix_listener_) {
        unix_listener_->stop();
    }
}

NV_NS_INFRA_NET_END
