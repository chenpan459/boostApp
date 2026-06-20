#include "service/gateway/router.hpp"

#include <sstream>

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

std::vector<std::string> Router::describe() const {
    std::vector<std::string> lines;
    lines.reserve(routes_.size());

    for (const auto& route : routes_) {
        std::ostringstream oss;
        oss << (route.method.empty() ? "*" : route.method) << ' '
            << (route.prefix ? (route.path + "*") : route.path) << " async="
            << (route.async ? "true" : "false");
        lines.push_back(oss.str());
    }

    if (lines.empty()) {
        lines.push_back("(no routes)");
    }
    return lines;
}

NV_NS_SERVICE_END
