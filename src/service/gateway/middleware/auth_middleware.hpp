#pragma once

#include "core/debug_stats.hpp"
#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

#include <string>

NV_NS_SERVICE_BEGIN

class AuthMiddleware : public domain::IMiddleware {
public:
    AuthMiddleware(std::string api_key, core::DebugStats* stats = nullptr);

    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;

private:
    std::string api_key_;
    core::DebugStats* stats_{nullptr};
};

NV_NS_SERVICE_END
