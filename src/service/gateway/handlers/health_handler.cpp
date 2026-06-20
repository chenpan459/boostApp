#include "service/gateway/handlers/health_handler.hpp"

NV_NS_SERVICE_BEGIN

void HealthHandler::handle(domain::GatewayContext& ctx) {
    ctx.response() = {200, "text/plain", "ok\n"};
}

NV_NS_SERVICE_END
