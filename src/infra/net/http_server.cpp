#include "infra/net/http_server.hpp"

#include "core/logger.hpp"
#include "domain/gateway/request.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <unistd.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

NV_NS_INFRA_NET_BEGIN
namespace {

std::string verb_to_string(http::verb method) {
    return std::string(http::to_string(method));
}

domain::GatewayRequest make_request(const http::request<http::string_body>& request,
                                    const std::string& client) {
    domain::GatewayRequest req;
    req.method = verb_to_string(request.method());
    req.path = std::string(request.target());
    req.body = request.body();
    req.client = client;
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    req.received_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    return req;
}

void send_gateway_response(http::response<http::string_body>& response,
                           const http::request<http::string_body>& request,
                           domain::GatewayResponse result) {
    response = {};
    response.version(request.version());
    response.result(static_cast<http::status>(result.status));
    response.set(http::field::server, "nvcomm-gateway");
    response.set(http::field::content_type, result.content_type);
    response.keep_alive(request.keep_alive());
    response.body() = std::move(result.body);
    response.prepare_payload();
}

class TcpGatewaySession : public std::enable_shared_from_this<TcpGatewaySession> {
public:
    TcpGatewaySession(beast::tcp_stream stream, service::Gateway& gateway, std::string client)
        : stream_(std::move(stream)), gateway_(gateway), client_(std::move(client)) {}

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
            beast::bind_front_handler(&TcpGatewaySession::on_read, shared_from_this()));
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

        auto packet = make_request(request_, client_);
        auto self = shared_from_this();
        gateway_.handle_async(std::move(packet), [self](domain::GatewayResponse result) {
            asio::post(self->stream_.get_executor(), [self, result = std::move(result)]() mutable {
                self->send_response(std::move(result));
            });
        });
    }

    void send_response(domain::GatewayResponse result) {
        send_gateway_response(response_, request_, std::move(result));
        stream_.expires_after(std::chrono::seconds(30));
        http::async_write(
            stream_,
            response_,
            beast::bind_front_handler(&TcpGatewaySession::on_write, shared_from_this(), response_.need_eof()));
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

    beast::tcp_stream stream_;
    service::Gateway& gateway_;
    std::string client_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
};

class UnixGatewaySession : public std::enable_shared_from_this<UnixGatewaySession> {
public:
    UnixGatewaySession(beast::basic_stream<asio::local::stream_protocol> stream, service::Gateway& gateway)
        : stream_(std::move(stream)), gateway_(gateway) {}

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
            beast::bind_front_handler(&UnixGatewaySession::on_read, shared_from_this()));
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

        auto packet = make_request(request_, "unix");
        auto self = shared_from_this();
        gateway_.handle_async(std::move(packet), [self](domain::GatewayResponse result) {
            asio::post(self->stream_.get_executor(), [self, result = std::move(result)]() mutable {
                self->send_response(std::move(result));
            });
        });
    }

    void send_response(domain::GatewayResponse result) {
        send_gateway_response(response_, request_, std::move(result));
        stream_.expires_after(std::chrono::seconds(30));
        http::async_write(
            stream_,
            response_,
            beast::bind_front_handler(&UnixGatewaySession::on_write, shared_from_this(), response_.need_eof()));
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

    beast::basic_stream<asio::local::stream_protocol> stream_;
    service::Gateway& gateway_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
};

}  // namespace

class HttpServer::TcpListener : public std::enable_shared_from_this<HttpServer::TcpListener> {
public:
    TcpListener(asio::io_context& io, tcp::endpoint endpoint, service::Gateway& gateway)
        : io_(io), acceptor_(io), gateway_(gateway) {
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

    void run() { do_accept(); }

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
            std::string client = "unknown";
            beast::error_code endpoint_ec;
            const auto endpoint = socket.remote_endpoint(endpoint_ec);
            if (!endpoint_ec) {
                client = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
            }
            std::make_shared<TcpGatewaySession>(beast::tcp_stream(std::move(socket)), gateway_, std::move(client))
                ->run();
        }
        do_accept();
    }

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    service::Gateway& gateway_;
};

class HttpServer::UnixListener : public std::enable_shared_from_this<HttpServer::UnixListener> {
public:
    UnixListener(asio::io_context& io, const std::string& path, service::Gateway& gateway)
        : io_(io), acceptor_(io), gateway_(gateway) {
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

    void run() { do_accept(); }

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
            std::make_shared<UnixGatewaySession>(
                beast::basic_stream<asio::local::stream_protocol>(std::move(socket)),
                gateway_)
                ->run();
        }
        do_accept();
    }

    asio::io_context& io_;
    asio::local::stream_protocol::acceptor acceptor_;
    service::Gateway& gateway_;
};

HttpServer::HttpServer(const core::AppConfig& config, IoContextPool& pool, service::Gateway& gateway)
    : config_(config), pool_(pool), gateway_(gateway) {}

void HttpServer::start() {
    auto& io = pool_.io_context();

    const auto address = asio::ip::make_address(config_.listen_address);
    tcp::endpoint endpoint(address, config_.listen_port);
    tcp_listener_ = std::make_shared<TcpListener>(io, endpoint, gateway_);
    tcp_listener_->run();
    BOOSTAPP_LOG(Info, "gateway listening on " + config_.listen_address + ":" + std::to_string(config_.listen_port));

    if (!config_.unix_socket.empty()) {
        unix_listener_ = std::make_shared<UnixListener>(io, config_.unix_socket, gateway_);
        unix_listener_->run();
        BOOSTAPP_LOG(Info, "gateway listening on unix://" + config_.unix_socket);
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
