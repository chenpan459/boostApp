#include "service/gateway_service.hpp"

#include "core/logger.hpp"
#include "service/gateway/handlers/health_handler.hpp"
#include "service/gateway/handlers/packet_handler.hpp"
#include "service/gateway/middleware/access_log_middleware.hpp"
#include "service/gateway/middleware/body_limit_middleware.hpp"

NV_NS_SERVICE_BEGIN

std::unique_ptr<Gateway> GatewayFactory::create(const core::AppConfig& config,
                                                boost::asio::thread_pool& workers) {
    auto gateway = std::make_unique<Gateway>(workers);
    const auto max_body_bytes = static_cast<std::size_t>(config.max_body_kb) * 1024;

    gateway->use(std::make_shared<AccessLogMiddleware>());
    gateway->use(std::make_shared<BodyLimitMiddleware>(max_body_bytes));

    gateway->add_route({.method = "GET",
                        .path = "/health",
                        .prefix = false,
                        .async = false,
                        .handler = std::make_shared<HealthHandler>()});

    gateway->add_route({.method = "POST",
                        .path = "/api/",
                        .prefix = true,
                        .async = true,
                        .handler = std::make_shared<PacketHandler>(config)});

    return gateway;
}

GatewayService::GatewayService(const core::AppConfig& config, std::atomic<bool>& running)
    : config_(config), running_(running) {}

void GatewayService::start() {
    const auto io_threads = config_.io_threads == 0 ? 1 : config_.io_threads;
    const auto worker_threads = config_.worker_threads == 0 ? 1 : config_.worker_threads;

    pool_ = std::make_unique<NV_NS_INFRA_NET::IoContextPool>(io_threads);
    workers_ = std::make_unique<boost::asio::thread_pool>(worker_threads);
    gateway_ = GatewayFactory::create(config_, *workers_);
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

NV_NS_SERVICE_END
