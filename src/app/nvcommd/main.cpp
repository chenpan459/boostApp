#include "core/application.hpp"
#include "core/logger.hpp"
#include "infra/ipc/message_queue_bus.hpp"
#include "infra/persistence/file_event_store.hpp"
#include "namespace.hpp"
#include "service/message_router.hpp"

#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    NV_NS_CORE::Application app(argc, argv, "nvcommd", true);

    return app.run([&](boost::asio::io_context&, std::atomic<bool>& running) {
        const auto& config = app.config();
        NV_NS_INFRA_IPC::MessageQueueBus::ensure_queues(config);

        auto inbound = NV_NS_INFRA_IPC::make_inbound_bus(config);
        auto outbound = NV_NS_INFRA_IPC::make_outbound_bus(config);

        std::unique_ptr<NV_NS_DOMAIN::IEventStore> event_store;
        if (config.persist) {
            event_store = std::make_unique<NV_NS_INFRA_PERSISTENCE::FileEventStore>(config.event_dir);
        }

        NV_NS_SERVICE::MessageRouter router(
            config,
            std::move(inbound),
            std::move(outbound),
            std::move(event_store));

        router.run(running);
    });
}
