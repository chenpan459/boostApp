#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

NV_NS_DOMAIN_BEGIN

struct Message {
    std::string topic;
    std::string payload;
    std::uint32_t priority{0};
};

using MessageHandler = std::function<void(const Message&)>;

class IMessageBus {
public:
    virtual ~IMessageBus() = default;

    virtual void publish(std::string_view topic, std::string_view payload, std::uint32_t priority = 0) = 0;
    virtual bool try_receive(Message& message, int timeout_ms) = 0;
};

NV_NS_DOMAIN_END
