#pragma once

#include "namespace.hpp"

NV_NS_PLATFORM_BEGIN

bool systemd_booted();
void notify_ready();
void notify_stopping();
void notify_watchdog();

NV_NS_PLATFORM_END
