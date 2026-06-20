#include "service/board_service.hpp"

#include "core/logger.hpp"

#include <sstream>
#include <thread>

NV_NS_SERVICE_BEGIN

BoardService::BoardService(core::AppConfig config,
                           std::unique_ptr<domain::IMessageBus> inbound,
                           std::unique_ptr<domain::IMessageBus> outbound)
    : config_(std::move(config)),
      inbound_(std::move(inbound)),
      outbound_(std::move(outbound)) {}

bool BoardService::should_forward(const domain::Message& message) const {
    return !message.topic.empty();
}

void BoardService::run(std::atomic<bool>& running) {
    BOOSTAPP_LOG(Info, "board service loop started");

    while (running.load()) {
        domain::Message message;
        if (!inbound_->try_receive(message, config_.poll_interval_ms)) {
            continue;
        }

        std::ostringstream oss;
        oss << "route topic=" << message.topic << " payload=" << message.payload
            << " priority=" << message.priority;
        BOOSTAPP_LOG(Info, oss.str());

        if (should_forward(message)) {
            outbound_->publish(message.topic, message.payload, message.priority);
        }
    }

    BOOSTAPP_LOG(Info, "board service loop stopped");
}

NV_NS_SERVICE_END
