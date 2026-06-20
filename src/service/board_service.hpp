#pragma once

#include "core/config.hpp"
#include "domain/ports/message_bus.hpp"

#include <atomic>
#include <memory>
#include <string>

namespace boostapp::service {

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

}  // namespace boostapp::service
