#include "platform/paths.hpp"

#include <filesystem>

NV_NS_PLATFORM_BEGIN

std::string config_dir() {
    if (std::filesystem::exists("/etc/nvcomm/nvcomm.conf")) {
        return "/etc/nvcomm";
    }
    return "config";
}

std::string default_config_path() {
    return (std::filesystem::path(config_dir()) / "nvcomm.conf").string();
}

std::string runtime_dir() {
    if (std::filesystem::exists("/run/nvcomm")) {
        return "/run/nvcomm";
    }
    return "run";
}

std::string data_dir() {
    if (std::filesystem::exists("/data/nvcomm")) {
        return "/data/nvcomm";
    }
    return "data";
}

NV_NS_PLATFORM_END
