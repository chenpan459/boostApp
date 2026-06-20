#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace boostapp::infra::ipc {

struct EncodedMessage {
    std::uint32_t priority{0};
    std::string topic;
    std::string payload;
};

std::vector<char> encode_message(std::string_view topic, std::string_view payload, std::uint32_t priority);
bool decode_message(const char* data, std::size_t size, EncodedMessage& out);

}  // namespace boostapp::infra::ipc
