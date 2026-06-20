#pragma once

#include "namespace.hpp"

#include <mutex>
#include <ostream>
#include <string>

NV_NS_CORE_BEGIN

enum class LogLevel { Trace, Debug, Info, Warning, Error, Fatal };

void init_logging(const std::string& log_dir, const std::string& level_name);
void shutdown_logging();

void log_write(LogLevel level, const std::string& message);

LogLevel parse_log_level(const std::string& level_name);

#define BOOSTAPP_LOG(level, msg) \
    NV_NS_CORE::log_write(NV_NS_CORE::LogLevel::level, (msg))

NV_NS_CORE_END
