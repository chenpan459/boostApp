#include "domain/gateway/context.hpp"
#include "service/gateway/middleware/auth_middleware.hpp"
#include "service/gateway/middleware/request_id_middleware.hpp"
#include "test_assert.hpp"

int test_middleware() {
    int failures = 0;

    NV_NS_DOMAIN::GatewayRequest req;
    req.method = "POST";
    req.path = "/api/v1/packet";
    req.client = "127.0.0.1:1";
    NV_NS_DOMAIN::GatewayContext ctx(std::move(req));

    NV_NS_SERVICE::RequestIdMiddleware request_id;
    request_id.process(ctx, [&ctx]() {
        ctx.response() = {200, "text/plain", "ok"};
    });
    EXPECT_TRUE(!ctx.request().request_id.empty());

    NV_NS_DOMAIN::GatewayRequest auth_req;
    auth_req.method = "POST";
    auth_req.path = "/api/v1/packet";
    auth_req.headers["x-api-key"] = "secret";
    NV_NS_DOMAIN::GatewayContext auth_ctx(std::move(auth_req));

    NV_NS_SERVICE::AuthMiddleware auth("secret");
    auth.process(auth_ctx, [&auth_ctx]() {
        auth_ctx.response() = {200, "application/json", "{}"};
    });
    EXPECT_TRUE(!auth_ctx.aborted());

    NV_NS_DOMAIN::GatewayRequest bad_req;
    bad_req.method = "POST";
    bad_req.path = "/api/v1/packet";
    bad_req.headers["x-api-key"] = "wrong";
    NV_NS_DOMAIN::GatewayContext bad_ctx(std::move(bad_req));

    auth.process(bad_ctx, []() {});
    EXPECT_TRUE(bad_ctx.aborted());
    EXPECT_EQ(bad_ctx.response().status, 401);

    return failures;
}
