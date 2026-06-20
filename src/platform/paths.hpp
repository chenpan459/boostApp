#pragma once

#include "namespace.hpp"

#include <string>

NV_NS_PLATFORM_BEGIN

std::string config_dir();
std::string default_config_path();
std::string runtime_dir();
std::string data_dir();

NV_NS_PLATFORM_END
