#include "infra/ipc/message_queue_bus.hpp"

#include "core/logger.hpp"
#include "infra/ipc/message_codec.hpp"

#include <boost/interprocess/ipc/message_queue.hpp>

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

NV_NS_INFRA_IPC_BEGIN
namespace bip = boost::interprocess;

namespace {

std::string queue_name_for(const core::AppConfig& config, QueueRole role) {
    return role == QueueRole::Inbound ? config.mq_in_name : config.mq_out_name;
}

void open_or_create_queue(const core::AppConfig& config, const std::string& name) {
    try {
        bip::message_queue::remove(name.c_str());
    } catch (...) {
    }

    bip::message_queue mq(
        bip::create_only,
        name.c_str(),
        config.max_messages,
        config.max_message_size);
}

}  // namespace

MessageQueueBus::MessageQueueBus(const core::AppConfig& config, QueueRole role)
    : config_(config), role_(role), queue_name_(queue_name_for(config, role)) {}

void MessageQueueBus::ensure_queues(const core::AppConfig& config) {
    open_or_create_queue(config, config.mq_in_name);
    open_or_create_queue(config, config.mq_out_name);
    BOOSTAPP_LOG(Info, "message queues ready: " + config.mq_in_name + ", " + config.mq_out_name);
}

void MessageQueueBus::remove_queues(const core::AppConfig& config) {
    bip::message_queue::remove(config.mq_in_name.c_str());
    bip::message_queue::remove(config.mq_out_name.c_str());
}

void MessageQueueBus::publish(std::string_view topic, std::string_view payload, std::uint32_t priority) {
    const auto encoded = encode_message(topic, payload, priority);
    if (encoded.size() > config_.max_message_size) {
        throw std::runtime_error("encoded message exceeds max_message_size");
    }

    bip::message_queue mq(bip::open_only, queue_name_.c_str());
    mq.send(encoded.data(), encoded.size(), priority);
}

bool MessageQueueBus::try_receive(domain::Message& message, int timeout_ms) {
    bip::message_queue mq(bip::open_only, queue_name_.c_str());
    std::vector<char> buffer(config_.max_message_size);
    std::size_t received_size = 0;
    unsigned int priority = 0;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (mq.try_receive(buffer.data(), buffer.size(), received_size, priority)) {
            EncodedMessage decoded;
            if (!decode_message(buffer.data(), received_size, decoded)) {
                BOOSTAPP_LOG(Warning, "dropped malformed message on queue " + queue_name_);
                return false;
            }
            message.topic = std::move(decoded.topic);
            message.payload = std::move(decoded.payload);
            message.priority = decoded.priority;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

std::unique_ptr<MessageQueueBus> make_inbound_bus(const core::AppConfig& config) {
    return std::make_unique<MessageQueueBus>(config, QueueRole::Inbound);
}

std::unique_ptr<MessageQueueBus> make_outbound_bus(const core::AppConfig& config) {
    return std::make_unique<MessageQueueBus>(config, QueueRole::Outbound);
}

NV_NS_INFRA_IPC_END
