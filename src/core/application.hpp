#pragma once

#include "core/config.hpp"
#include "namespace.hpp"
#include "platform/watchdog.hpp"
#include "service/gateway_service.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <memory>
#include <string>

NV_NS_INFRA_CLI_BEGIN
class CliServer;
NV_NS_INFRA_CLI_END

NV_NS_CORE_BEGIN

class Application {
public:
    Application(int argc, char** argv, std::string app_name, bool enable_platform = false);
    ~Application();

    int run_network();

    const AppConfig& config() const { return config_; }
    boost::asio::io_context& io_context() { return io_context_; }
    std::atomic<bool>& running() { return running_; }

private:
    void parse_args(int argc, char** argv);
    void setup_signals();
    void setup_platform();
    void schedule_watchdog_feed();

    std::string app_name_;
    bool enable_platform_{false};
    std::string config_path_;
    AppConfig config_;
    boost::asio::io_context io_context_;
    boost::asio::signal_set signals_;
    boost::asio::steady_timer watchdog_timer_;
    std::unique_ptr<platform::Watchdog> watchdog_;
    std::unique_ptr<service::GatewayService> gateway_service_;
    std::unique_ptr<NV_NS_INFRA_CLI::CliServer> cli_server_;
    std::atomic<bool> running_{true};
    int exit_code_{0};
};

NV_NS_CORE_END
