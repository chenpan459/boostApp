#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>

NV_NS_DOMAIN_BEGIN

struct GatewayRequest {
    std::string method;
    std::string path;
    std::string body;
    std::string client;
    std::uint64_t received_ns{0};
};

NV_NS_DOMAIN_END
