#pragma once

#include "core/config.hpp"
#include "domain/ports/message_bus.hpp"
#include "namespace.hpp"

#include <atomic>
#include <memory>
#include <string>

NV_NS_SERVICE_BEGIN

class BoardService {
public:
    BoardService(core::AppConfig config,
                 std::unique_ptr<domain::IMessageBus> inbound,
                 std::unique_ptr<domain::IMessageBus> outbound);

    void run(std::atomic<bool>& running);

private:
    bool should_forward(const domain::Message& message) const;

    core::AppConfig config_;
    std::unique_ptr<domain::IMessageBus> inbound_;
    std::unique_ptr<domain::IMessageBus> outbound_;
};

NV_NS_SERVICE_END
