#include "service/gateway/gateway.hpp"

#include <boost/asio/post.hpp>

#include <utility>

NV_NS_SERVICE_BEGIN

Gateway::Gateway(boost::asio::thread_pool& workers) : workers_(workers) {}

void Gateway::use(std::shared_ptr<domain::IMiddleware> middleware) {
    middlewares_.push_back(std::move(middleware));
}

void Gateway::add_route(Route route) {
    router_.add(std::move(route));
}

void Gateway::run_pipeline(std::shared_ptr<domain::GatewayContext> ctx,
                           std::size_t index,
                           std::function<void()> on_done) {
    if (index < middlewares_.size()) {
        middlewares_[index]->process(*ctx, [this, ctx, index, on_done = std::move(on_done)]() mutable {
            if (ctx->aborted()) {
                on_done();
                return;
            }
            run_pipeline(std::move(ctx), index + 1, std::move(on_done));
        });
        return;
    }

    const Route* route = router_.match(ctx->request());
    if (!route || !route->handler) {
        ctx->abort({404, "application/json", R"({"error":"not found"})"});
        on_done();
        return;
    }

    if (route->async) {
        boost::asio::post(workers_.get_executor(),
                          [handler = route->handler, ctx = std::move(ctx), on_done = std::move(on_done)]() mutable {
                              handler->handle(*ctx);
                              on_done();
                          });
        return;
    }

    route->handler->handle(*ctx);
    on_done();
}

domain::GatewayResponse Gateway::handle_sync(domain::GatewayRequest request) {
    auto ctx = std::make_shared<domain::GatewayContext>(std::move(request));
    run_pipeline(ctx, 0, [] {});
    return ctx->response();
}

void Gateway::handle_async(domain::GatewayRequest request,
                           std::function<void(domain::GatewayResponse)> on_complete) {
    auto ctx = std::make_shared<domain::GatewayContext>(std::move(request));
    run_pipeline(ctx, 0, [ctx, on_complete = std::move(on_complete)]() mutable {
        on_complete(std::move(ctx->response()));
    });
}

std::vector<std::string> Gateway::describe_routes() const {
    return router_.describe();
}

NV_NS_SERVICE_END
