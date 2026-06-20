#pragma once

#include "domain/ports/handler.hpp"
#include "namespace.hpp"

#include <memory>
#include <string>
#include <vector>

NV_NS_SERVICE_BEGIN

struct Route {
    std::string method;
    std::string path;
    bool prefix{false};
    bool async{false};
    std::shared_ptr<domain::IHandler> handler;
};

class Router {
public:
    void add(Route route);
    const Route* match(const domain::GatewayRequest& request) const;

private:
    std::vector<Route> routes_;
};

NV_NS_SERVICE_END
