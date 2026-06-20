#pragma once

#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

NV_NS_SERVICE_BEGIN

class AccessLogMiddleware : public domain::IMiddleware {
public:
    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;
};

NV_NS_SERVICE_END
