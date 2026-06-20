#include "platform/watchdog.hpp"

#include "core/logger.hpp"

#include <fcntl.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

NV_NS_PLATFORM_BEGIN

Watchdog::Watchdog() = default;

Watchdog::~Watchdog() {
    close();
}

bool Watchdog::open() {
    if (fd_ >= 0) {
        return true;
    }

    fd_ = ::open("/dev/watchdog", O_WRONLY | O_CLOEXEC);
    if (fd_ < 0) {
        BOOSTAPP_LOG(Warning, std::string("watchdog unavailable: ") + std::strerror(errno));
        return false;
    }

    BOOSTAPP_LOG(Info, "hardware watchdog opened");
    return true;
}

void Watchdog::close() {
    if (fd_ < 0) {
        return;
    }

    const int magic = WDIOS_DISABLECARD;
    ioctl(fd_, WDIOC_SETOPTIONS, &magic);
    ::close(fd_);
    fd_ = -1;
}

void Watchdog::feed() {
    if (fd_ < 0) {
        return;
    }

    const int rc = ioctl(fd_, WDIOC_KEEPALIVE, nullptr);
    if (rc != 0) {
        BOOSTAPP_LOG(Warning, std::string("watchdog feed failed: ") + std::strerror(errno));
    }
}

NV_NS_PLATFORM_END
