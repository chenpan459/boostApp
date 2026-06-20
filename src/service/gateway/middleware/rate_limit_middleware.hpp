#pragma once

#include "core/debug_stats.hpp"
#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

NV_NS_SERVICE_BEGIN

class RateLimitMiddleware : public domain::IMiddleware {
public:
    RateLimitMiddleware(std::uint32_t requests_per_second, core::DebugStats* stats = nullptr);

    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;

private:
    std::uint32_t requests_per_second_{0};
    core::DebugStats* stats_{nullptr};
    std::mutex mutex_;
    std::unordered_map<std::string, std::pair<std::int64_t, std::uint32_t>> counters_;
};

NV_NS_SERVICE_END
