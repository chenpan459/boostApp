#pragma once

#include "core/config.hpp"
#include "infra/net/io_context_pool.hpp"
#include "namespace.hpp"
#include "service/packet_processor.hpp"

NV_NS_INFRA_NET_BEGIN

class HttpServer {
public:
    HttpServer(const core::AppConfig& config,
               IoContextPool& pool,
               service::PacketProcessor& processor);

    void start();
    void stop();

private:
    class TcpListener;
    class UnixListener;

    const core::AppConfig& config_;
    IoContextPool& pool_;
    service::PacketProcessor& processor_;
    std::shared_ptr<TcpListener> tcp_listener_;
    std::shared_ptr<UnixListener> unix_listener_;
};

NV_NS_INFRA_NET_END
