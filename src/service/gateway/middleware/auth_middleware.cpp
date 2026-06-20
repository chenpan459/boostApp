#include "service/gateway/middleware/auth_middleware.hpp"

#include <string_view>

NV_NS_SERVICE_BEGIN

AuthMiddleware::AuthMiddleware(std::string api_key) : api_key_(std::move(api_key)) {}

void AuthMiddleware::process(domain::GatewayContext& ctx, domain::NextMiddleware next) {
    if (api_key_.empty() || ctx.request().path == "/health") {
        next();
        return;
    }

    const auto& headers = ctx.request().headers;
    std::string provided;
    if (const auto it = headers.find("x-api-key"); it != headers.end()) {
        provided = it->second;
    } else if (const auto it = headers.find("authorization"); it != headers.end()) {
        constexpr std::string_view prefix = "Bearer ";
        if (it->second.rfind(prefix.data(), 0) == 0) {
            provided = it->second.substr(prefix.size());
        }
    }

    if (provided != api_key_) {
        ctx.abort({401, "application/json", R"({"error":"unauthorized"})"});
        return;
    }

    next();
}

NV_NS_SERVICE_END
