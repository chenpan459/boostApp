#pragma once

#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

#include <cstddef>

NV_NS_SERVICE_BEGIN

class BodyLimitMiddleware : public domain::IMiddleware {
public:
    explicit BodyLimitMiddleware(std::size_t max_body_bytes);

    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;

private:
    std::size_t max_body_bytes_;
};

NV_NS_SERVICE_END
