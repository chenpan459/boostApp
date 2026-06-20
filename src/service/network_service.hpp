#pragma once

#include "core/config.hpp"
#include "infra/net/io_context_pool.hpp"
#include "infra/net/http_server.hpp"
#include "namespace.hpp"
#include "service/packet_processor.hpp"

#include <boost/asio/thread_pool.hpp>

#include <atomic>
#include <memory>
#include <thread>

NV_NS_SERVICE_BEGIN

class NetworkService {
public:
    NetworkService(const core::AppConfig& config, std::atomic<bool>& running);

    void start();
    void stop();
    void wait();

private:
    const core::AppConfig& config_;
    std::atomic<bool>& running_;
    std::unique_ptr<NV_NS_INFRA_NET::IoContextPool> pool_;
    std::unique_ptr<boost::asio::thread_pool> workers_;
    std::unique_ptr<PacketProcessor> processor_;
    std::unique_ptr<NV_NS_INFRA_NET::HttpServer> http_server_;
    std::thread pool_thread_;
};

NV_NS_SERVICE_END
