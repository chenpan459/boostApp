#include "service/gateway/middleware/rate_limit_middleware.hpp"

#include <chrono>

NV_NS_SERVICE_BEGIN

RateLimitMiddleware::RateLimitMiddleware(std::uint32_t requests_per_second)
    : requests_per_second_(requests_per_second) {}

void RateLimitMiddleware::process(domain::GatewayContext& ctx, domain::NextMiddleware next) {
    if (requests_per_second_ == 0 || ctx.request().path == "/health") {
        next();
        return;
    }

    const auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
                             std::chrono::steady_clock::now().time_since_epoch())
                             .count();

    const auto& client = ctx.request().client;
    {
        std::lock_guard lock(mutex_);
        auto& entry = counters_[client];
        if (entry.first != now_sec) {
            entry = {now_sec, 0};
        }
        ++entry.second;
        if (entry.second > requests_per_second_) {
            ctx.abort({429, "application/json", R"({"error":"rate limit exceeded"})"});
            return;
        }
    }

    next();
}

NV_NS_SERVICE_END
