#include "core/request_id.hpp"

#include <chrono>
#include <random>
#include <sstream>

NV_NS_CORE_BEGIN

std::string generate_request_id() {
    static thread_local std::mt19937_64 rng{
        static_cast<std::mt19937_64::result_type>(
            std::chrono::steady_clock::now().time_since_epoch().count())};

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::uniform_int_distribution<std::uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << ms << '-' << dist(rng);
    return oss.str();
}

NV_NS_CORE_END
