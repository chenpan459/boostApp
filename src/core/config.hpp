#pragma once

#include <cstdint>
#include <string>

namespace boostapp::core {

struct AppConfig {
    std::string mq_in_name{"boardcomm_in"};
    std::string mq_out_name{"boardcomm_out"};
    std::uint32_t max_messages{128};
    std::uint32_t max_message_size{4096};
    std::string log_dir{"log"};
    std::string log_level{"info"};
    int poll_interval_ms{50};
};

AppConfig load_config(const std::string& path);

}  // namespace boostapp::core
