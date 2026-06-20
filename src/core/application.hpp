#pragma once

#include "core/config.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace boostapp::core {

class Application {
public:
    using WorkLoop = std::function<void(boost::asio::io_context&, std::atomic<bool>&)>;

    Application(int argc, char** argv, std::string app_name);
    ~Application();

    int run(WorkLoop work_loop);

    const AppConfig& config() const { return config_; }
    boost::asio::io_context& io_context() { return io_context_; }

private:
    void parse_args(int argc, char** argv);
    void setup_signals();

    std::string app_name_;
    std::string config_path_{"config/boardcomm.json"};
    AppConfig config_;
    boost::asio::io_context io_context_;
    boost::asio::signal_set signals_;
    std::atomic<bool> running_{true};
    int exit_code_{0};
};

}  // namespace boostapp::core
