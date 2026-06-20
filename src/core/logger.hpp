#pragma once

#include <mutex>
#include <ostream>
#include <string>

namespace boostapp::core {

enum class LogLevel { Trace, Debug, Info, Warning, Error, Fatal };

void init_logging(const std::string& log_dir, const std::string& level_name);
void shutdown_logging();

void log_write(LogLevel level, const std::string& message);

LogLevel parse_log_level(const std::string& level_name);

#define BOOSTAPP_LOG(level, msg) \
    ::boostapp::core::log_write(::boostapp::core::LogLevel::level, (msg))

}  // namespace boostapp::core
