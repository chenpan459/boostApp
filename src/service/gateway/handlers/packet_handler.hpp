#pragma once

#include "core/config.hpp"
#include "domain/ports/event_store.hpp"
#include "domain/ports/handler.hpp"
#include "namespace.hpp"

#include <memory>

NV_NS_SERVICE_BEGIN

class PacketHandler : public domain::IHandler {
public:
    explicit PacketHandler(const core::AppConfig& config);

    void handle(domain::GatewayContext& ctx) override;

private:
    core::AppConfig config_;
    std::unique_ptr<domain::IEventStore> event_store_;
};

NV_NS_SERVICE_END
