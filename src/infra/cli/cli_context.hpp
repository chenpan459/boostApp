#pragma once

#include "core/config.hpp"
#include "core/debug_stats.hpp"
#include "namespace.hpp"
#include "service/gateway/gateway.hpp"

#include <atomic>
#include <chrono>
#include <string>

NV_NS_INFRA_CLI_BEGIN

struct CliContext {
    const core::AppConfig* config{nullptr};
    core::DebugStats* stats{nullptr};
    const service::Gateway* gateway{nullptr};
    std::chrono::steady_clock::time_point started_at{};
    std::atomic<bool>* running{nullptr};
    std::string config_path;
};

NV_NS_INFRA_CLI_END
