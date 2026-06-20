#pragma once

#include "namespace.hpp"

#include <string>
#include <utility>

NV_NS_CORE_BEGIN

std::pair<std::string, std::string> split_path_query(std::string_view target);

NV_NS_CORE_END
