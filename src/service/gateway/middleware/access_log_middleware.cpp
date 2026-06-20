#include "service/gateway/middleware/access_log_middleware.hpp"

#include "core/logger.hpp"

#include <sstream>

NV_NS_SERVICE_BEGIN

AccessLogMiddleware::AccessLogMiddleware(core::DebugStats* stats) : stats_(stats) {}

void AccessLogMiddleware::process(domain::GatewayContext& ctx, domain::NextMiddleware next) {
    if (stats_) {
        ++stats_->total_requests;
    }

    next();

    const auto& req = ctx.request();
    const auto& res = ctx.response();

    if (stats_) {
        if (res.status >= 400) {
            ++stats_->error_responses;
        } else {
            ++stats_->success_responses;
        }
    }

    std::ostringstream line;
    line << req.request_id << ' ' << req.method << ' ' << req.path;
    if (!req.query.empty()) {
        line << '?' << req.query;
    }
    line << ' ' << res.status << ' ' << req.client;
    BOOSTAPP_LOG(Info, "gateway " + line.str());
}

NV_NS_SERVICE_END
