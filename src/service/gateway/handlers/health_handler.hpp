#pragma once

#include "domain/ports/handler.hpp"
#include "namespace.hpp"

NV_NS_SERVICE_BEGIN

class HealthHandler : public domain::IHandler {
public:
    void handle(domain::GatewayContext& ctx) override;
};

NV_NS_SERVICE_END
