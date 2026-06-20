#pragma once

#include "domain/gateway/context.hpp"
#include "namespace.hpp"

NV_NS_DOMAIN_BEGIN

class IHandler {
public:
    virtual ~IHandler() = default;
    virtual void handle(GatewayContext& ctx) = 0;
};

NV_NS_DOMAIN_END
