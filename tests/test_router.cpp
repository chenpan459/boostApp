#include "service/gateway/router.hpp"
#include "test_assert.hpp"

#include "service/gateway/handlers/health_handler.hpp"

int test_router() {
    int failures = 0;

    NV_NS_SERVICE::Router router;
    router.add({.method = "GET",
                .path = "/health",
                .prefix = false,
                .async = false,
                .handler = std::make_shared<NV_NS_SERVICE::HealthHandler>()});
    router.add({.method = "POST",
                .path = "/api/",
                .prefix = true,
                .async = true,
                .handler = std::make_shared<NV_NS_SERVICE::HealthHandler>()});

    NV_NS_DOMAIN::GatewayRequest req;
    req.method = "GET";
    req.path = "/health";
    EXPECT_TRUE(router.match(req) != nullptr);

    req.path = "/health?probe=1";
    const auto* no_match = router.match(req);
    EXPECT_TRUE(no_match == nullptr);

    req.method = "POST";
    req.path = "/api/v1/packet";
    EXPECT_TRUE(router.match(req) != nullptr);

    req.path = "/other";
    EXPECT_TRUE(router.match(req) == nullptr);

    return failures;
}
