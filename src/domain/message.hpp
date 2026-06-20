#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>

NV_NS_DOMAIN_BEGIN

struct Message {
    std::string topic;
    std::string payload;
    std::uint32_t priority{0};
    std::uint64_t timestamp_ns{0};
    std::string source;
};

NV_NS_DOMAIN_END
