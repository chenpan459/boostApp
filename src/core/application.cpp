#include "core/application.hpp"

#include "core/logger.hpp"

#include <csignal>
#include <iostream>
#include <sstream>
#include <thread>

NV_NS_CORE_BEGIN

Application::Application(int argc, char** argv, std::string app_name)
    : app_name_(std::move(app_name)), signals_(io_context_, SIGINT, SIGTERM) {
    parse_args(argc, argv);
    config_ = load_config(config_path_);
    init_logging(config_.log_dir, config_.log_level);
    setup_signals();
}

Application::~Application() {
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
                 << "  -c, --config PATH   config file (default: config/boardcomm.conf)\n"
                 << "  -h, --help          show help\n";
            std::cout << help.str();
            std::exit(0);
        }
    }
}

void Application::setup_signals() {
    signals_.async_wait([this](const boost::system::error_code& ec, int signo) {
        if (ec) {
            return;
        }
        BOOSTAPP_LOG(Info, "received signal " + std::to_string(signo) + ", shutting down");
        running_.store(false);
        io_context_.stop();
    });
}

int Application::run(WorkLoop work_loop) {
    BOOSTAPP_LOG(Info, app_name_ + " starting");

    std::thread worker([&] {
        work_loop(io_context_, running_);
    });

    io_context_.run();
    running_.store(false);

    if (worker.joinable()) {
        worker.join();
    }

    BOOSTAPP_LOG(Info, app_name_ + " stopped");
    return exit_code_;
}

NV_NS_CORE_END
