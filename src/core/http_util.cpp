#include "core/http_util.hpp"

NV_NS_CORE_BEGIN

std::pair<std::string, std::string> split_path_query(std::string_view target) {
    const auto pos = target.find('?');
    if (pos == std::string_view::npos) {
        return {std::string(target), {}};
    }
    return {std::string(target.substr(0, pos)), std::string(target.substr(pos + 1))};
}

NV_NS_CORE_END
