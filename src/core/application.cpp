#include "core/application.hpp"

#include "core/logger.hpp"
#include "platform/cpu_affinity.hpp"
#include "platform/paths.hpp"
#include "platform/systemd_notify.hpp"
#include "platform/watchdog.hpp"
#include "service/network_service.hpp"

#include <csignal>
#include <iostream>
#include <memory>
#include <sstream>

NV_NS_CORE_BEGIN
namespace {

std::unique_ptr<platform::Watchdog> g_watchdog;
std::unique_ptr<service::NetworkService> g_network_service;

}  // namespace

Application::Application(int argc, char** argv, std::string app_name, bool enable_platform)
    : app_name_(std::move(app_name)),
      enable_platform_(enable_platform),
      config_path_(platform::default_config_path()),
      signals_(io_context_, SIGINT, SIGTERM),
      watchdog_timer_(io_context_) {
    parse_args(argc, argv);
    config_ = load_config(config_path_);
    init_logging(config_.log_dir, config_.log_level);
    setup_signals();
    if (enable_platform_) {
        setup_platform();
    }
}

Application::~Application() {
    if (enable_platform_) {
        platform::notify_stopping();
        g_watchdog.reset();
    }
    shutdown_logging();
}

void Application::parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path_ = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::ostringstream help;
            help << "Usage: " << app_name_ << " [options]\n"
                 << "  -c, --config PATH   config file (default: " << config_path_ << ")\n"
                 << "  -h, --help          show help\n";
            std::cout << help.str();
            std::exit(0);
        }
    }
}

void Application::setup_platform() {
    platform::bind_current_thread_to_cpu(config_.cpu_affinity);

    if (config_.watchdog_sec > 0) {
        g_watchdog = std::make_unique<platform::Watchdog>();
        g_watchdog->open();
    }
}

void Application::setup_signals() {
    signals_.async_wait([this](const boost::system::error_code& ec, int signo) {
        if (ec) {
            return;
        }
        BOOSTAPP_LOG(Info, "received signal " + std::to_string(signo) + ", shutting down");
        running_.store(false);
        if (g_network_service) {
            g_network_service->stop();
        }
        io_context_.stop();
    });
}

void Application::schedule_watchdog_feed() {
    if (!enable_platform_) {
        return;
    }

    const int interval_sec = config_.watchdog_sec > 0 ? config_.watchdog_sec / 2 : 0;
    if (interval_sec <= 0 && !platform::systemd_booted()) {
        return;
    }

    const int timer_sec = interval_sec > 0 ? interval_sec : 5;
    watchdog_timer_.expires_after(std::chrono::seconds(timer_sec));
    watchdog_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (ec || !running_.load()) {
            return;
        }

        if (g_watchdog && g_watchdog->is_open()) {
            g_watchdog->feed();
        }
        if (platform::systemd_booted()) {
            platform::notify_watchdog();
        }
        schedule_watchdog_feed();
    });
}

int Application::run_network() {
    BOOSTAPP_LOG(Info, app_name_ + " starting network service");

    if (enable_platform_) {
        platform::notify_ready();
        schedule_watchdog_feed();
    }

    g_network_service = std::make_unique<service::NetworkService>(config_, running_);
    g_network_service->start();

    io_context_.run();
    running_.store(false);
    g_network_service->stop();
    g_network_service.reset();

    BOOSTAPP_LOG(Info, app_name_ + " stopped");
    return exit_code_;
}

NV_NS_CORE_END
