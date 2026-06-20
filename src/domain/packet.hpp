#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>

NV_NS_DOMAIN_BEGIN

struct NetworkPacket {
    std::string path;
    std::string body;
    std::string client;
    std::uint64_t received_ns{0};
};

struct PacketResult {
    int status_code{200};
    std::string content_type{"application/json"};
    std::string body;
};

NV_NS_DOMAIN_END
