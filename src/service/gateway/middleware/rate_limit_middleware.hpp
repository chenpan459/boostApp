#pragma once

#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

NV_NS_SERVICE_BEGIN

class RateLimitMiddleware : public domain::IMiddleware {
public:
    explicit RateLimitMiddleware(std::uint32_t requests_per_second);

    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;

private:
    std::uint32_t requests_per_second_{0};
    std::mutex mutex_;
    std::unordered_map<std::string, std::pair<std::int64_t, std::uint32_t>> counters_;
};

NV_NS_SERVICE_END
