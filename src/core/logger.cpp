#include "core/logger.hpp"

#include <cstdint>
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
std::string g_log_dir;
std::uint64_t g_max_file_bytes{10ULL * 1024 * 1024};

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

std::string timestamp_for_filename() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&time, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

std::string make_log_path(const std::string& log_dir) {
    return (std::filesystem::path(log_dir) / ("nvcomm_" + timestamp_for_filename() + ".log")).string();
}

void open_log_file_unlocked() {
    if (g_file && g_file->is_open()) {
        g_file->flush();
        g_file->close();
    }

    const auto log_path = make_log_path(g_log_dir);
    g_file = std::make_unique<std::ofstream>(log_path, std::ios::app);
    if (!g_file->is_open()) {
        std::cerr << "failed to open log file: " << log_path << '\n';
        g_file.reset();
    }
}

void rotate_if_needed_unlocked(std::size_t next_write_size) {
    if (!g_file || !g_file->is_open()) {
        open_log_file_unlocked();
        return;
    }

    const auto pos = g_file->tellp();
    if (pos < 0) {
        return;
    }

    if (static_cast<std::uint64_t>(pos) + next_write_size > g_max_file_bytes) {
        open_log_file_unlocked();
    }
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

void init_logging(const std::string& log_dir,
                  const std::string& level_name,
                  std::uint32_t max_file_size_mb) {
    std::lock_guard lock(g_log_mutex);
    g_min_level = parse_log_level(level_name);
    g_log_dir = log_dir;
    g_max_file_bytes = static_cast<std::uint64_t>(max_file_size_mb == 0 ? 10 : max_file_size_mb) * 1024 * 1024;
    std::filesystem::create_directories(g_log_dir);
    open_log_file_unlocked();
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
    const std::string text = line.str();

    std::lock_guard lock(g_log_mutex);
    std::cerr << text << '\n';
    if (g_file && g_file->is_open()) {
        rotate_if_needed_unlocked(text.size() + 1);
        if (g_file && g_file->is_open()) {
            (*g_file) << text << '\n';
            g_file->flush();
        }
    }
}

NV_NS_CORE_END
