#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>

NV_NS_CORE_BEGIN

struct AppConfig {
    std::string mq_in_name{"nvcomm_in"};
    std::string mq_out_name{"nvcomm_out"};
    std::uint32_t max_messages{256};
    std::uint32_t max_message_size{4096};

    std::string log_dir{"log"};
    std::string log_level{"info"};
    std::uint32_t log_max_size_mb{32};

    int poll_interval_ms{20};
    int watchdog_sec{0};
    int cpu_affinity{-1};

    std::string event_dir{"data/events"};
    bool persist{true};
};

AppConfig load_config(const std::string& path);

NV_NS_CORE_END
