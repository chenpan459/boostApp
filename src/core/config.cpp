#include "core/config.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <stdexcept>

namespace boostapp::core {

AppConfig load_config(const std::string& path) {
    boost::property_tree::ptree tree;
    try {
        boost::property_tree::read_json(path, tree);
    } catch (const std::exception& ex) {
        throw std::runtime_error("failed to load config: " + std::string(ex.what()));
    }

    AppConfig cfg;
    cfg.mq_in_name = tree.get<std::string>("mq_in_name", cfg.mq_in_name);
    cfg.mq_out_name = tree.get<std::string>("mq_out_name", cfg.mq_out_name);
    cfg.max_messages = tree.get<std::uint32_t>("max_messages", cfg.max_messages);
    cfg.max_message_size = tree.get<std::uint32_t>("max_message_size", cfg.max_message_size);
    cfg.log_dir = tree.get<std::string>("log_dir", cfg.log_dir);
    cfg.log_level = tree.get<std::string>("log_level", cfg.log_level);
    cfg.poll_interval_ms = tree.get<int>("poll_interval_ms", cfg.poll_interval_ms);
    return cfg;
}

}  // namespace boostapp::core
