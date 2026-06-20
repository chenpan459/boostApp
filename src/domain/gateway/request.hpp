#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

NV_NS_DOMAIN_BEGIN

struct GatewayRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::string client;
    std::string request_id;
    std::uint64_t received_ns{0};
    std::unordered_map<std::string, std::string> headers;
};

NV_NS_DOMAIN_END
