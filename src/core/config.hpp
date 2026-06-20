#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>

NV_NS_CORE_BEGIN

struct AppConfig {
    std::string listen_address{"0.0.0.0"};
    std::uint16_t listen_port{8080};
    std::uint32_t io_threads{4};
    std::uint32_t worker_threads{4};
    std::uint32_t max_body_kb{1024};
    std::string unix_socket;

    std::string log_dir{"log"};
    std::string log_level{"info"};
    std::uint32_t log_max_size_mb{10};

    std::string gateway_api_key;
    std::uint32_t gateway_rate_limit_rps{0};

    bool cli_enabled{true};
    std::string cli_socket{"run/nvcomm.cli.sock"};

    int watchdog_sec{0};
    int cpu_affinity{-1};

    std::string event_dir{"data/events"};
    bool persist{true};
    std::uint32_t event_max_size_mb{50};
};

AppConfig load_config(const std::string& path);

NV_NS_CORE_END
