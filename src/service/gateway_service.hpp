#pragma once

#include "core/config.hpp"
#include "core/debug_stats.hpp"
#include "domain/ports/event_store.hpp"
#include "infra/cli/cli_context.hpp"
#include "infra/net/http_server.hpp"
#include "infra/net/io_context_pool.hpp"
#include "namespace.hpp"
#include "service/gateway/gateway.hpp"

#include <boost/asio/thread_pool.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

NV_NS_SERVICE_BEGIN

class GatewayFactory {
public:
    static std::unique_ptr<Gateway> create(const core::AppConfig& config,
                                           boost::asio::thread_pool& workers,
                                           core::DebugStats& stats,
                                           std::shared_ptr<domain::IEventStore> event_store);
};

class GatewayService {
public:
    GatewayService(const core::AppConfig& config, std::atomic<bool>& running);

    void start();
    void stop();
    void wait();

    NV_NS_INFRA_CLI::CliContext cli_context(const std::string& config_path) const;

private:
    const core::AppConfig& config_;
    std::atomic<bool>& running_;
    core::DebugStats stats_;
    std::chrono::steady_clock::time_point started_at_{};
    std::unique_ptr<NV_NS_INFRA_NET::IoContextPool> pool_;
    std::unique_ptr<boost::asio::thread_pool> workers_;
    std::unique_ptr<Gateway> gateway_;
    std::unique_ptr<NV_NS_INFRA_NET::HttpServer> http_server_;
    std::thread pool_thread_;
};

NV_NS_SERVICE_END
