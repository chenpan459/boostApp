#include "service/gateway/middleware/access_log_middleware.hpp"

#include "core/logger.hpp"

#include <sstream>

NV_NS_SERVICE_BEGIN

void AccessLogMiddleware::process(domain::GatewayContext& ctx, domain::NextMiddleware next) {
    next();

    const auto& req = ctx.request();
    const auto& res = ctx.response();
    std::ostringstream line;
    line << req.method << ' ' << req.path << ' ' << res.status << ' ' << req.client;
    BOOSTAPP_LOG(Info, "gateway " + line.str());
}

NV_NS_SERVICE_END
