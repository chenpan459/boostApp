#pragma once

#include "core/config.hpp"
#include "domain/ports/event_store.hpp"
#include "domain/ports/message_bus.hpp"
#include "namespace.hpp"

#include <atomic>
#include <memory>

NV_NS_SERVICE_BEGIN

class MessageRouter {
public:
    MessageRouter(core::AppConfig config,
                  std::unique_ptr<domain::IMessageBus> inbound,
                  std::unique_ptr<domain::IMessageBus> outbound,
                  std::unique_ptr<domain::IEventStore> event_store = nullptr);

    void run(std::atomic<bool>& running);

private:
    bool should_forward(const domain::Message& message) const;
    domain::Message enrich_message(domain::Message message) const;

    core::AppConfig config_;
    std::unique_ptr<domain::IMessageBus> inbound_;
    std::unique_ptr<domain::IMessageBus> outbound_;
    std::unique_ptr<domain::IEventStore> event_store_;
};

NV_NS_SERVICE_END
