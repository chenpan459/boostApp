#include "service/message_router.hpp"

#include "core/logger.hpp"

#include <chrono>
#include <sstream>

NV_NS_SERVICE_BEGIN

MessageRouter::MessageRouter(core::AppConfig config,
                             std::unique_ptr<domain::IMessageBus> inbound,
                             std::unique_ptr<domain::IMessageBus> outbound,
                             std::unique_ptr<domain::IEventStore> event_store)
    : config_(std::move(config)),
      inbound_(std::move(inbound)),
      outbound_(std::move(outbound)),
      event_store_(std::move(event_store)) {}

bool MessageRouter::should_forward(const domain::Message& message) const {
    return !message.topic.empty();
}

domain::Message MessageRouter::enrich_message(domain::Message message) const {
    if (message.timestamp_ns == 0) {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        message.timestamp_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    }
    if (message.source.empty()) {
        message.source = "nvcommd";
    }
    return message;
}

void MessageRouter::run(std::atomic<bool>& running) {
    BOOSTAPP_LOG(Info, "message router loop started");

    while (running.load()) {
        domain::Message message;
        if (!inbound_->try_receive(message, config_.poll_interval_ms)) {
            continue;
        }

        message = enrich_message(std::move(message));

        std::ostringstream oss;
        oss << "route topic=" << message.topic
            << " payload=" << message.payload
            << " priority=" << message.priority
            << " source=" << message.source;
        BOOSTAPP_LOG(Info, oss.str());

        if (config_.persist && event_store_) {
            event_store_->append(message);
        }

        if (should_forward(message)) {
            outbound_->publish(message.topic, message.payload, message.priority);
        }
    }

    BOOSTAPP_LOG(Info, "message router loop stopped");
}

NV_NS_SERVICE_END
