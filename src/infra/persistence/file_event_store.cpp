#include "infra/persistence/file_event_store.hpp"

#include "core/logger.hpp"

#include <chrono>
#include <filesystem>
#include <sstream>

NV_NS_INFRA_PERSISTENCE_BEGIN

FileEventStore::FileEventStore(std::string event_dir)
    : event_dir_(std::move(event_dir)),
      event_file_((std::filesystem::path(event_dir_) / "events.log").string()) {
    std::filesystem::create_directories(event_dir_);
    stream_.open(event_file_, std::ios::app);
    if (!stream_.is_open()) {
        throw std::runtime_error("failed to open event store: " + event_file_);
    }
    BOOSTAPP_LOG(Info, "event store ready: " + event_file_);
}

void FileEventStore::append(const domain::Message& message) {
    std::lock_guard lock(mutex_);
    stream_ << message.timestamp_ns << '\t'
            << message.priority << '\t'
            << message.source << '\t'
            << message.topic << '\t'
            << message.payload << '\n';
    stream_.flush();
}

NV_NS_INFRA_PERSISTENCE_END
