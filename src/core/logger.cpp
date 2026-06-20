#include "core/logger.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

NV_NS_CORE_BEGIN
namespace {

std::mutex g_log_mutex;
LogLevel g_min_level{LogLevel::Info};
std::unique_ptr<std::ofstream> g_file;

const char* level_name(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
    }
    return "UNKNOWN";
}

std::string now_string() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&time, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace

LogLevel parse_log_level(const std::string& level_name) {
    if (level_name == "trace") return LogLevel::Trace;
    if (level_name == "debug") return LogLevel::Debug;
    if (level_name == "info") return LogLevel::Info;
    if (level_name == "warning" || level_name == "warn") return LogLevel::Warning;
    if (level_name == "error") return LogLevel::Error;
    if (level_name == "fatal") return LogLevel::Fatal;
    return LogLevel::Info;
}

void init_logging(const std::string& log_dir, const std::string& level_name) {
    g_min_level = parse_log_level(level_name);
    std::filesystem::create_directories(log_dir);
    const auto log_path = (std::filesystem::path(log_dir) / "nvcomm.log").string();
    g_file = std::make_unique<std::ofstream>(log_path, std::ios::app);
}

void shutdown_logging() {
    std::lock_guard lock(g_log_mutex);
    if (g_file && g_file->is_open()) {
        g_file->flush();
        g_file.reset();
    }
}

void log_write(LogLevel level, const std::string& message) {
    if (level < g_min_level) {
        return;
    }

    std::ostringstream line;
    line << now_string() << " [" << level_name(level) << "] " << message;

    std::lock_guard lock(g_log_mutex);
    std::cerr << line.str() << '\n';
    if (g_file && g_file->is_open()) {
        (*g_file) << line.str() << '\n';
        g_file->flush();
    }
}

NV_NS_CORE_END
