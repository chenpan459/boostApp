#include "core/config.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

NV_NS_CORE_BEGIN
namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string make_key(const std::string& section, const std::string& key) {
    return section.empty() ? key : section + "." + key;
}

std::unordered_map<std::string, std::string> parse_conf_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open config: " + path);
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    std::string section;
    std::size_t line_no = 0;

    while (std::getline(input, line)) {
        ++line_no;
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            throw std::runtime_error("invalid config line " + std::to_string(line_no) + ": " + line);
        }

        const auto key = trim(line.substr(0, pos));
        const auto value = trim(line.substr(pos + 1));
        if (key.empty()) {
            throw std::runtime_error("empty config key at line " + std::to_string(line_no));
        }

        values[make_key(section, key)] = value;
    }

    return values;
}

template <typename T>
T get_value(const std::unordered_map<std::string, std::string>& values,
            const std::string& section,
            const std::string& key,
            T fallback);

template <>
std::string get_value<std::string>(const std::unordered_map<std::string, std::string>& values,
                                   const std::string& section,
                                   const std::string& key,
                                   std::string fallback) {
    const auto qualified = make_key(section, key);
    const auto it = values.find(qualified);
    if (it != values.end()) {
        return it->second;
    }
    const auto flat = values.find(key);
    return flat == values.end() ? fallback : flat->second;
}

template <>
std::uint32_t get_value<std::uint32_t>(const std::unordered_map<std::string, std::string>& values,
                                       const std::string& section,
                                       const std::string& key,
                                       std::uint32_t fallback) {
    const auto text = get_value<std::string>(values, section, key, std::string{});
    if (text.empty()) {
        return fallback;
    }
    try {
        return static_cast<std::uint32_t>(std::stoul(text));
    } catch (const std::exception& ex) {
        throw std::runtime_error("invalid uint32 value for '" + make_key(section, key) + "': " + ex.what());
    }
}

template <>
int get_value<int>(const std::unordered_map<std::string, std::string>& values,
                   const std::string& section,
                   const std::string& key,
                   int fallback) {
    const auto text = get_value<std::string>(values, section, key, std::string{});
    if (text.empty()) {
        return fallback;
    }
    try {
        return std::stoi(text);
    } catch (const std::exception& ex) {
        throw std::runtime_error("invalid int value for '" + make_key(section, key) + "': " + ex.what());
    }
}

template <>
bool get_value<bool>(const std::unordered_map<std::string, std::string>& values,
                     const std::string& section,
                     const std::string& key,
                     bool fallback) {
    const auto text = get_value<std::string>(values, section, key, std::string{});
    if (text.empty()) {
        return fallback;
    }
    if (text == "1" || text == "true" || text == "yes" || text == "on") {
        return true;
    }
    if (text == "0" || text == "false" || text == "no" || text == "off") {
        return false;
    }
    throw std::runtime_error("invalid bool value for '" + make_key(section, key) + "': " + text);
}

}  // namespace

AppConfig load_config(const std::string& path) {
    const auto values = parse_conf_file(path);

    AppConfig cfg;
    cfg.mq_in_name = get_value(values, "mq", "in_name", cfg.mq_in_name);
    cfg.mq_out_name = get_value(values, "mq", "out_name", cfg.mq_out_name);
    cfg.max_messages = get_value(values, "mq", "max_messages", cfg.max_messages);
    cfg.max_message_size = get_value(values, "mq", "max_message_size", cfg.max_message_size);

    cfg.log_dir = get_value(values, "log", "dir", cfg.log_dir);
    cfg.log_level = get_value(values, "log", "level", cfg.log_level);
    cfg.log_max_size_mb = get_value(values, "log", "max_size_mb", cfg.log_max_size_mb);

    cfg.poll_interval_ms = get_value(values, "runtime", "poll_interval_ms", cfg.poll_interval_ms);
    cfg.watchdog_sec = get_value(values, "runtime", "watchdog_sec", cfg.watchdog_sec);
    cfg.cpu_affinity = get_value(values, "runtime", "cpu_affinity", cfg.cpu_affinity);

    cfg.event_dir = get_value(values, "data", "event_dir", cfg.event_dir);
    cfg.persist = get_value(values, "data", "persist", cfg.persist);
    return cfg;
}

NV_NS_CORE_END
