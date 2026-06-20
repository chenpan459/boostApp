#pragma once

#include "domain/gateway/context.hpp"
#include "domain/gateway/request.hpp"
#include "domain/gateway/response.hpp"
#include "domain/ports/middleware.hpp"
#include "namespace.hpp"
#include "service/gateway/router.hpp"

#include <boost/asio/thread_pool.hpp>

#include <functional>
#include <memory>
#include <vector>

NV_NS_SERVICE_BEGIN

class Gateway {
public:
    Gateway(boost::asio::thread_pool& workers);

    void use(std::shared_ptr<domain::IMiddleware> middleware);
    void add_route(Route route);

    domain::GatewayResponse handle_sync(domain::GatewayRequest request);
    void handle_async(domain::GatewayRequest request,
                      std::function<void(domain::GatewayResponse)> on_complete);

private:
    void run_pipeline(std::shared_ptr<domain::GatewayContext> ctx,
                      std::size_t index,
                      std::function<void()> on_done);

    boost::asio::thread_pool& workers_;
    Router router_;
    std::vector<std::shared_ptr<domain::IMiddleware>> middlewares_;
};

NV_NS_SERVICE_END
