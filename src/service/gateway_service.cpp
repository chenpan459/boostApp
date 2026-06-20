#include "service/gateway_service.hpp"

#include "core/logger.hpp"
#include "infra/persistence/file_event_store.hpp"
#include "service/gateway/handlers/health_handler.hpp"
#include "service/gateway/handlers/packet_handler.hpp"
#include "service/gateway/middleware/access_log_middleware.hpp"
#include "service/gateway/middleware/auth_middleware.hpp"
#include "service/gateway/middleware/body_limit_middleware.hpp"
#include "service/gateway/middleware/rate_limit_middleware.hpp"
#include "service/gateway/middleware/request_id_middleware.hpp"

NV_NS_SERVICE_BEGIN

std::unique_ptr<Gateway> GatewayFactory::create(const core::AppConfig& config,
                                                boost::asio::thread_pool& workers,
                                                core::DebugStats& stats,
                                                std::shared_ptr<domain::IEventStore> event_store) {
    auto gateway = std::make_unique<Gateway>(workers);
    const auto max_body_bytes = static_cast<std::size_t>(config.max_body_kb) * 1024;

    gateway->use(std::make_shared<RequestIdMiddleware>());
    gateway->use(std::make_shared<RateLimitMiddleware>(config.gateway_rate_limit_rps, &stats));
    gateway->use(std::make_shared<AuthMiddleware>(config.gateway_api_key, &stats));
    gateway->use(std::make_shared<BodyLimitMiddleware>(max_body_bytes));
    gateway->use(std::make_shared<AccessLogMiddleware>(&stats));

    gateway->add_route({.method = "GET",
                        .path = "/health",
                        .prefix = false,
                        .async = false,
                        .handler = std::make_shared<HealthHandler>()});

    gateway->add_route({.method = "POST",
                        .path = "/api/",
                        .prefix = true,
                        .async = true,
                        .handler = std::make_shared<PacketHandler>(config, std::move(event_store))});

    return gateway;
}

GatewayService::GatewayService(const core::AppConfig& config, std::atomic<bool>& running)
    : config_(config), running_(running) {}

void GatewayService::start() {
    const auto io_threads = config_.io_threads == 0 ? 1 : config_.io_threads;
    const auto worker_threads = config_.worker_threads == 0 ? 1 : config_.worker_threads;

    started_at_ = std::chrono::steady_clock::now();
    pool_ = std::make_unique<NV_NS_INFRA_NET::IoContextPool>(io_threads);
    workers_ = std::make_unique<boost::asio::thread_pool>(worker_threads);

    std::shared_ptr<domain::IEventStore> event_store;
    if (config_.persist) {
        event_store = std::make_shared<infra::persistence::FileEventStore>(
            config_.event_dir, config_.event_max_size_mb, *workers_);
    }

    gateway_ = GatewayFactory::create(config_, *workers_, stats_, std::move(event_store));
    http_server_ = std::make_unique<NV_NS_INFRA_NET::HttpServer>(config_, *pool_, *gateway_);
    http_server_->start();

    pool_thread_ = std::thread([this] {
        pool_->run();
    });

    BOOSTAPP_LOG(Info,
                 "gateway started io_threads=" + std::to_string(io_threads) +
                     " worker_threads=" + std::to_string(worker_threads));
}

void GatewayService::stop() {
    if (http_server_) {
        http_server_->stop();
    }
    if (pool_) {
        pool_->stop();
    }
    if (workers_) {
        workers_->stop();
    }
    if (pool_thread_.joinable()) {
        pool_thread_.join();
    }
    if (workers_) {
        workers_->join();
    }
    BOOSTAPP_LOG(Info, "gateway stopped");
}

void GatewayService::wait() {
    if (pool_thread_.joinable()) {
        pool_thread_.join();
    }
}

NV_NS_INFRA_CLI::CliContext GatewayService::cli_context(const std::string& config_path) const {
    return NV_NS_INFRA_CLI::CliContext{
        .config = &config_,
        .stats = const_cast<core::DebugStats*>(&stats_),
        .gateway = gateway_.get(),
        .started_at = started_at_,
        .running = &running_,
        .config_path = config_path,
    };
}

NV_NS_SERVICE_END
