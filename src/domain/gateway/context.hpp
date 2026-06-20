#pragma once

#include "domain/gateway/request.hpp"
#include "domain/gateway/response.hpp"
#include "namespace.hpp"

NV_NS_DOMAIN_BEGIN

class GatewayContext {
public:
    explicit GatewayContext(GatewayRequest request)
        : request_(std::move(request)) {}

    const GatewayRequest& request() const { return request_; }
    GatewayRequest& request() { return request_; }

    const GatewayResponse& response() const { return response_; }
    GatewayResponse& response() { return response_; }

    bool aborted() const { return aborted_; }

    void abort(GatewayResponse response) {
        response_ = std::move(response);
        aborted_ = true;
    }

private:
    GatewayRequest request_;
    GatewayResponse response_;
    bool aborted_{false};
};

NV_NS_DOMAIN_END
