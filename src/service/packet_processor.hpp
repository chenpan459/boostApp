#pragma once

#include "core/config.hpp"
#include "domain/packet.hpp"
#include "domain/ports/event_store.hpp"
#include "namespace.hpp"

#include <boost/asio/thread_pool.hpp>

#include <functional>
#include <memory>

NV_NS_SERVICE_BEGIN

class PacketProcessor {
public:
    PacketProcessor(const core::AppConfig& config, boost::asio::thread_pool& workers);

    void process_async(domain::NetworkPacket packet,
                       std::function<void(domain::PacketResult)> on_complete);

    domain::PacketResult process_sync(const domain::NetworkPacket& packet);

private:
    core::AppConfig config_;
    boost::asio::thread_pool& workers_;
    std::unique_ptr<domain::IEventStore> event_store_;
};

NV_NS_SERVICE_END
