#pragma once

#include "domain/ports/event_store.hpp"
#include "namespace.hpp"

#include <fstream>
#include <mutex>
#include <string>

NV_NS_INFRA_PERSISTENCE_BEGIN

class FileEventStore : public domain::IEventStore {
public:
    explicit FileEventStore(std::string event_dir);

    void append(const domain::Message& message) override;

private:
    std::string event_dir_;
    std::string event_file_;
    std::mutex mutex_;
    std::ofstream stream_;
};

NV_NS_INFRA_PERSISTENCE_END
