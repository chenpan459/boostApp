#pragma once

#include "domain/ports/middleware.hpp"
#include "namespace.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

NV_NS_SERVICE_BEGIN

class AuthMiddleware : public domain::IMiddleware {
public:
    explicit AuthMiddleware(std::string api_key);

    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override;

private:
    std::string api_key_;
};

NV_NS_SERVICE_END
