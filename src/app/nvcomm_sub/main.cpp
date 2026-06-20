#include "core/application.hpp"
#include "core/logger.hpp"
#include "domain/ports/message_bus.hpp"
#include "infra/ipc/message_queue_bus.hpp"
#include "namespace.hpp"

#include <atomic>
#include <iostream>
#include <string>

namespace {

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [topic_filter] [options]\n"
              << "  topic_filter        optional; only print matching topic\n"
              << "  -c, --config PATH   config file\n"
              << "Example: " << prog << " sensor/temp\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string topic_filter;
    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        if (arg == "-c" || arg == "--config") {
            ++i;
            continue;
        }
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        if (topic_filter.empty()) {
            topic_filter = arg;
        }
    }

    NV_NS_CORE::Application app(argc, argv, "nvcomm_sub");
    auto bus = NV_NS_INFRA_IPC::make_outbound_bus(app.config());

    std::cout << "subscribed on queue '" << app.config().mq_out_name << "'";
    if (!topic_filter.empty()) {
        std::cout << " filter='" << topic_filter << "'";
    }
    std::cout << " (Ctrl+C to stop)\n";

    return app.run([&](boost::asio::io_context&, std::atomic<bool>& app_running) {
        while (app_running.load()) {
            NV_NS_DOMAIN::Message message;
            if (!bus->try_receive(message, app.config().poll_interval_ms)) {
                continue;
            }

            if (!topic_filter.empty() && message.topic != topic_filter) {
                continue;
            }

            std::cout << "[" << message.topic << "] " << message.payload << '\n';
            BOOSTAPP_LOG(Info, "received topic=" + message.topic + " payload=" + message.payload);
        }
    });
}
