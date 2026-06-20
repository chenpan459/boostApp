#include "service/gateway/handlers/packet_handler.hpp"

#include "core/json_util.hpp"
#include "core/logger.hpp"

#include <chrono>
#include <sstream>

NV_NS_SERVICE_BEGIN

PacketHandler::PacketHandler(const core::AppConfig& config, std::shared_ptr<domain::IEventStore> event_store)
    : config_(config), event_store_(std::move(event_store)) {}

void PacketHandler::handle(domain::GatewayContext& ctx) {
    const auto& req = ctx.request();

    if (req.body.empty()) {
        ctx.abort({400, "application/json", R"({"error":"empty body"})"});
        return;
    }

    if (config_.persist && event_store_) {
        domain::Message message;
        message.topic = req.path;
        message.payload = req.body;
        message.source = req.client;
        message.request_id = req.request_id;
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        message.timestamp_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
        event_store_->append(message);
    }

    std::ostringstream json;
    json << R"({"status":"ok","request_id":")" << NV_NS_CORE::json_escape(req.request_id) << R"(","path":")"
         << NV_NS_CORE::json_escape(req.path) << R"(","bytes":)" << req.body.size() << R"(,"client":")"
         << NV_NS_CORE::json_escape(req.client) << R"("})";

    BOOSTAPP_LOG(Info,
                 "packet handled id=" + req.request_id + " path=" + req.path +
                     " bytes=" + std::to_string(req.body.size()));
    ctx.response() = {200, "application/json", json.str()};
}

NV_NS_SERVICE_END
