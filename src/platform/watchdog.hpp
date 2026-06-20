#pragma once

#include "namespace.hpp"

NV_NS_PLATFORM_BEGIN

class Watchdog {
public:
    Watchdog();
    ~Watchdog();

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

    bool open();
    void close();
    bool is_open() const { return fd_ >= 0; }
    void feed();

private:
    int fd_{-1};
};

NV_NS_PLATFORM_END
