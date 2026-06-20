#pragma once

#include "domain/ports/event_store.hpp"
#include "namespace.hpp"

#include <boost/asio/thread_pool.hpp>

#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>

NV_NS_INFRA_PERSISTENCE_BEGIN

class FileEventStore : public domain::IEventStore {
public:
    FileEventStore(std::string event_dir,
                   std::uint32_t max_file_size_mb,
                   boost::asio::thread_pool& writers);

    void append(const domain::Message& message) override;

private:
    void append_sync(const domain::Message& message);
    void rotate_if_needed_unlocked(std::size_t next_write_size);
    void open_log_file_unlocked();

    std::string event_dir_;
    std::uint64_t max_file_bytes_{50ULL * 1024 * 1024};
    boost::asio::thread_pool& writers_;
    std::mutex mutex_;
    std::ofstream stream_;
};

NV_NS_INFRA_PERSISTENCE_END
