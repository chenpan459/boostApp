#include "platform/systemd_notify.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

NV_NS_PLATFORM_BEGIN
namespace {

bool send_notify(const char* message) {
    const char* socket_path = std::getenv("NOTIFY_SOCKET");
    if (socket_path == nullptr || socket_path[0] == '\0') {
        return false;
    }

    const int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return false;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    address.sun_path[0] = '\0';
    std::strncpy(address.sun_path + 1, socket_path, sizeof(address.sun_path) - 2);

    const std::size_t path_len = std::strlen(socket_path);
    const socklen_t address_len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + 1 + path_len);

    const ssize_t sent = sendto(
        fd,
        message,
        std::strlen(message),
        MSG_NOSIGNAL,
        reinterpret_cast<sockaddr*>(&address),
        address_len);
    ::close(fd);
    return sent >= 0;
}

}  // namespace

bool systemd_booted() {
    const char* socket_path = std::getenv("NOTIFY_SOCKET");
    return socket_path != nullptr && socket_path[0] != '\0';
}

void notify_ready() {
    send_notify("READY=1");
}

void notify_stopping() {
    send_notify("STOPPING=1");
}

void notify_watchdog() {
    send_notify("WATCHDOG=1");
}

NV_NS_PLATFORM_END
