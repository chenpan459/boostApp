#pragma once

#include "namespace.hpp"

#include <atomic>
#include <cstdint>

NV_NS_CORE_BEGIN

struct DebugStats {
    std::atomic<std::uint64_t> total_requests{0};
    std::atomic<std::uint64_t> success_responses{0};
    std::atomic<std::uint64_t> error_responses{0};
    std::atomic<std::uint64_t> rate_limited{0};
    std::atomic<std::uint64_t> unauthorized{0};
    std::atomic<std::uint64_t> cli_commands{0};
};

NV_NS_CORE_END
