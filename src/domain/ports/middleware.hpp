#pragma once

#include "domain/gateway/context.hpp"
#include "namespace.hpp"

#include <functional>

NV_NS_DOMAIN_BEGIN

using NextMiddleware = std::function<void()>;

class IMiddleware {
public:
    virtual ~IMiddleware() = default;
    virtual void process(GatewayContext& ctx, NextMiddleware next) = 0;
};

NV_NS_DOMAIN_END
