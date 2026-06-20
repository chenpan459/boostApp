#pragma once

#include "namespace.hpp"

#include <string>

NV_NS_DOMAIN_BEGIN

struct GatewayResponse {
    int status{200};
    std::string content_type{"application/json"};
    std::string body;
};

NV_NS_DOMAIN_END
