#include "service/network_service.hpp"

#include "core/logger.hpp"
#include "infra/net/http_server.hpp"
#include "infra/net/io_context_pool.hpp"

#include <boost/asio/thread_pool.hpp>

NV_NS_SERVICE_BEGIN

NetworkService::NetworkService(const core::AppConfig& config, std::atomic<bool>& running)
    : config_(config), running_(running) {}

void NetworkService::start() {
    const auto io_threads = config_.io_threads == 0 ? 1 : config_.io_threads;
    const auto worker_threads = config_.worker_threads == 0 ? 1 : config_.worker_threads;

    pool_ = std::make_unique<NV_NS_INFRA_NET::IoContextPool>(io_threads);
    workers_ = std::make_unique<boost::asio::thread_pool>(worker_threads);
    processor_ = std::make_unique<PacketProcessor>(config_, *workers_);
    http_server_ = std::make_unique<NV_NS_INFRA_NET::HttpServer>(config_, *pool_, *processor_);
    http_server_->start();

    pool_thread_ = std::thread([this] {
        pool_->run();
    });

    BOOSTAPP_LOG(Info,
                 "network service started io_threads=" + std::to_string(io_threads) +
                     " worker_threads=" + std::to_string(worker_threads));
}

void NetworkService::stop() {
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
    BOOSTAPP_LOG(Info, "network service stopped");
}

void NetworkService::wait() {
    if (pool_thread_.joinable()) {
        pool_thread_.join();
    }
}

NV_NS_SERVICE_END
