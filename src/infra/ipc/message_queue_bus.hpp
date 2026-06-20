#pragma once

#include "core/config.hpp"
#include "domain/ports/message_bus.hpp"

#include <memory>
#include <string>

namespace boostapp::infra::ipc {

enum class QueueRole { Inbound, Outbound };

class MessageQueueBus : public domain::IMessageBus {
public:
    MessageQueueBus(const core::AppConfig& config, QueueRole role);

    void publish(std::string_view topic, std::string_view payload, std::uint32_t priority = 0) override;
    bool try_receive(domain::Message& message, int timeout_ms) override;

    static void ensure_queues(const core::AppConfig& config);
    static void remove_queues(const core::AppConfig& config);

private:
    core::AppConfig config_;
    QueueRole role_;
    std::string queue_name_;
};

std::unique_ptr<MessageQueueBus> make_inbound_bus(const core::AppConfig& config);
std::unique_ptr<MessageQueueBus> make_outbound_bus(const core::AppConfig& config);

}  // namespace boostapp::infra::ipc
