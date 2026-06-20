#include "service/gateway/middleware/request_id_middleware.hpp"

#include "core/request_id.hpp"

NV_NS_SERVICE_BEGIN

void RequestIdMiddleware::process(domain::GatewayContext& ctx, domain::NextMiddleware next) {
    if (ctx.request().request_id.empty()) {
        ctx.set_request_id(NV_NS_CORE::generate_request_id());
    }
    next();
}

NV_NS_SERVICE_END
