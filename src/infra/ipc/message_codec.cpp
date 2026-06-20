#include "infra/ipc/message_codec.hpp"

#include <cstring>
#include <stdexcept>

namespace boostapp::infra::ipc {
namespace {

constexpr std::size_t kHeaderSize = sizeof(std::uint32_t) + sizeof(std::uint16_t);

}  // namespace

std::vector<char> encode_message(std::string_view topic, std::string_view payload, std::uint32_t priority) {
    if (topic.size() > 65535 || payload.size() > 65535) {
        throw std::runtime_error("message topic or payload too large");
    }

    const auto topic_len = static_cast<std::uint16_t>(topic.size());
    const auto payload_len = static_cast<std::uint16_t>(payload.size());
    std::vector<char> buffer(kHeaderSize + topic.size() + payload.size());

    std::memcpy(buffer.data(), &priority, sizeof(priority));
    std::memcpy(buffer.data() + sizeof(priority), &topic_len, sizeof(topic_len));
    std::memcpy(buffer.data() + kHeaderSize, topic.data(), topic.size());
    std::memcpy(buffer.data() + kHeaderSize + topic.size(), payload.data(), payload.size());
    (void)payload_len;
    return buffer;
}

bool decode_message(const char* data, std::size_t size, EncodedMessage& out) {
    if (size < kHeaderSize) {
        return false;
    }

    std::uint32_t priority = 0;
    std::uint16_t topic_len = 0;
    std::memcpy(&priority, data, sizeof(priority));
    std::memcpy(&topic_len, data + sizeof(priority), sizeof(topic_len));

    if (size < kHeaderSize + topic_len) {
        return false;
    }

    out.priority = priority;
    out.topic.assign(data + kHeaderSize, topic_len);
    out.payload.assign(data + kHeaderSize + topic_len, size - kHeaderSize - topic_len);
    return true;
}

}  // namespace boostapp::infra::ipc
