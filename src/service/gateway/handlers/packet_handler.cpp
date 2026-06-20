#include "service/gateway/handlers/packet_handler.hpp"

#include "core/logger.hpp"
#include "infra/persistence/file_event_store.hpp"

#include <chrono>
#include <sstream>

NV_NS_SERVICE_BEGIN

PacketHandler::PacketHandler(const core::AppConfig& config) : config_(config) {
    if (config_.persist) {
        event_store_ = std::make_unique<infra::persistence::FileEventStore>(config_.event_dir);
    }
}

void PacketHandler::handle(domain::GatewayContext& ctx) {
    const auto& req = ctx.request();

    if (req.method != "POST") {
        ctx.abort({405, "application/json", R"({"error":"POST required"})"});
        return;
    }

    if (req.body.empty()) {
        ctx.abort({400, "application/json", R"({"error":"empty body"})"});
        return;
    }

    if (config_.persist && event_store_) {
        domain::Message message;
        message.topic = req.path;
        message.payload = req.body;
        message.source = req.client;
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        message.timestamp_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
        event_store_->append(message);
    }

    std::ostringstream json;
    json << R"({"status":"ok","path":")" << req.path << R"(","bytes":)" << req.body.size()
         << R"(,"client":")" << req.client << R"("})";

    BOOSTAPP_LOG(Info, "packet handled path=" + req.path + " bytes=" + std::to_string(req.body.size()));
    ctx.response() = {200, "application/json", json.str()};
}

NV_NS_SERVICE_END
