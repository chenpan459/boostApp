#pragma once

#include "core/debug_stats.hpp"
#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

NV_NS_SERVICE_BEGIN

class AccessLogMiddleware : public domain::IMiddleware {
public:
    explicit AccessLogMiddleware(core::DebugStats* stats = nullptr);

    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;

private:
    core::DebugStats* stats_{nullptr};
};

NV_NS_SERVICE_END
