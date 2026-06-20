#include "service/packet_processor.hpp"

#include "core/logger.hpp"
#include "infra/persistence/file_event_store.hpp"

#include <boost/asio/post.hpp>

#include <chrono>
#include <sstream>

NV_NS_SERVICE_BEGIN

PacketProcessor::PacketProcessor(const core::AppConfig& config, boost::asio::thread_pool& workers)
    : config_(config), workers_(workers) {
    if (config_.persist) {
        event_store_ = std::make_unique<infra::persistence::FileEventStore>(config_.event_dir);
    }
}

void PacketProcessor::process_async(domain::NetworkPacket packet,
                                    std::function<void(domain::PacketResult)> on_complete) {
    boost::asio::post(workers_.get_executor(), [this, packet = std::move(packet), on_complete = std::move(on_complete)]() mutable {
        auto result = process_sync(packet);
        on_complete(std::move(result));
    });
}

domain::PacketResult PacketProcessor::process_sync(const domain::NetworkPacket& packet) {
    if (packet.path == "/health") {
        return {200, "text/plain", "ok\n"};
    }

    if (packet.body.empty()) {
        return {400, "application/json", R"({"error":"empty body"})"};
    }

    if (packet.body.size() > static_cast<std::size_t>(config_.max_body_kb) * 1024) {
        return {413, "application/json", R"({"error":"payload too large"})"};
    }

    if (config_.persist && event_store_) {
        domain::Message message;
        message.topic = packet.path;
        message.payload = packet.body;
        message.source = packet.client;
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        message.timestamp_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
        event_store_->append(message);
    }

    std::ostringstream json;
    json << R"({"status":"ok","path":")" << packet.path << R"(","bytes":)" << packet.body.size()
         << R"(,"client":")" << packet.client << R"("})";

    BOOSTAPP_LOG(Info, "processed packet path=" + packet.path + " bytes=" + std::to_string(packet.body.size()));
    return {200, "application/json", json.str()};
}

NV_NS_SERVICE_END
