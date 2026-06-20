#include "service/gateway/middleware/body_limit_middleware.hpp"

NV_NS_SERVICE_BEGIN

BodyLimitMiddleware::BodyLimitMiddleware(std::size_t max_body_bytes)
    : max_body_bytes_(max_body_bytes) {}

void BodyLimitMiddleware::process(domain::GatewayContext& ctx, domain::NextMiddleware next) {
    if (ctx.request().body.size() > max_body_bytes_) {
        ctx.abort({413, "application/json", R"({"error":"payload too large"})"});
        return;
    }
    next();
}

NV_NS_SERVICE_END
