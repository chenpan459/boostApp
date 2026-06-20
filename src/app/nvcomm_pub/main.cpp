#include "core/application.hpp"
#include "core/logger.hpp"
#include "infra/ipc/message_queue_bus.hpp"
#include "namespace.hpp"

#include <iostream>
#include <string>

namespace {

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " <topic> <payload> [options]\n"
              << "  -c, --config PATH   config file\n"
              << "Example: " << prog << " sensor/temp 25.3\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string topic;
    std::string payload;

    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        if (arg == "-c" || arg == "--config") {
            ++i;
            continue;
        }
        if (topic.empty()) {
            topic = arg;
        } else if (payload.empty()) {
            payload = arg;
        }
    }

    if (topic.empty() || payload.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    NV_NS_CORE::Application app(argc, argv, "nvcomm_pub");
    auto bus = NV_NS_INFRA_IPC::make_inbound_bus(app.config());

    try {
        bus->publish(topic, payload);
        BOOSTAPP_LOG(Info, "published topic=" + topic + " payload=" + payload);
        std::cout << "published [" << topic << "] " << payload << '\n';
    } catch (const std::exception& ex) {
        std::cerr << "publish failed: " << ex.what() << '\n';
        std::cerr << "hint: start nvcommd first to create message queues\n";
        return 1;
    }

    return 0;
}
