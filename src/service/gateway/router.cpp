#include "service/gateway/router.hpp"

NV_NS_SERVICE_BEGIN

void Router::add(Route route) {
    routes_.push_back(std::move(route));
}

const Route* Router::match(const domain::GatewayRequest& request) const {
    for (const auto& route : routes_) {
        if (!route.method.empty() && route.method != request.method) {
            continue;
        }

        if (route.prefix) {
            if (request.path.rfind(route.path, 0) == 0) {
                return &route;
            }
            continue;
        }

        if (request.path == route.path) {
            return &route;
        }
    }
    return nullptr;
}

NV_NS_SERVICE_END
