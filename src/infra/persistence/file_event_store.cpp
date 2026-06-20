#include "infra/persistence/file_event_store.hpp"

#include "core/logger.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

NV_NS_INFRA_PERSISTENCE_BEGIN
namespace {

std::string timestamp_for_filename() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&time, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

}  // namespace

FileEventStore::FileEventStore(std::string event_dir,
                               std::uint32_t max_file_size_mb,
                               boost::asio::thread_pool& writers)
    : event_dir_(std::move(event_dir)),
      max_file_bytes_(static_cast<std::uint64_t>(max_file_size_mb == 0 ? 50 : max_file_size_mb) * 1024 * 1024),
      writers_(writers) {
    std::filesystem::create_directories(event_dir_);
    open_log_file_unlocked();
    BOOSTAPP_LOG(Info, "event store ready: " + event_dir_);
}

void FileEventStore::open_log_file_unlocked() {
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }

    const auto path = (std::filesystem::path(event_dir_) / ("events_" + timestamp_for_filename() + ".log")).string();
    stream_.open(path, std::ios::app);
    if (!stream_.is_open()) {
        throw std::runtime_error("failed to open event store: " + path);
    }
}

void FileEventStore::rotate_if_needed_unlocked(std::size_t next_write_size) {
    if (!stream_.is_open()) {
        open_log_file_unlocked();
        return;
    }

    const auto pos = stream_.tellp();
    if (pos >= 0 && static_cast<std::uint64_t>(pos) + next_write_size > max_file_bytes_) {
        open_log_file_unlocked();
    }
}

void FileEventStore::append_sync(const domain::Message& message) {
    std::lock_guard lock(mutex_);

    std::ostringstream line;
    line << message.timestamp_ns << '\t'
         << message.priority << '\t'
         << message.source << '\t'
         << message.topic << '\t'
         << message.request_id << '\t'
         << message.payload << '\n';
    const auto text = line.str();

    rotate_if_needed_unlocked(text.size());
    stream_ << text;
    stream_.flush();
}

void FileEventStore::append(const domain::Message& message) {
    boost::asio::post(writers_.get_executor(), [this, message]() {
        append_sync(message);
    });
}

NV_NS_INFRA_PERSISTENCE_END
