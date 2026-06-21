#include "infra/cli/cli_dispatcher.hpp"

#include "core/config.hpp"
#include "core/logger.hpp"

#include <chrono>
#include <sstream>
#include <vector>

NV_NS_INFRA_CLI_BEGIN
namespace {

std::vector<std::string> split_words(const std::string& line) {
    std::vector<std::string> words;
    std::istringstream input(line);
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    return words;
}

std::string format_uptime(std::chrono::steady_clock::time_point started_at) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - started_at);
    const auto hours = elapsed.count() / 3600;
    const auto minutes = (elapsed.count() % 3600) / 60;
    const auto seconds = elapsed.count() % 60;

    std::ostringstream oss;
    oss << hours << 'h' << minutes << 'm' << seconds << 's';
    return oss.str();
}

}  // namespace

CliDispatcher::CliDispatcher(CliContext context) : context_(std::move(context)) {}

std::vector<std::string> CliDispatcher::dispatch(std::string line) {
    if (context_.stats) {
        ++context_.stats->cli_commands;
    }

    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }

    if (line.empty()) {
        return {};
    }

    const auto words = split_words(line);
    const auto& cmd = words.front();

    if (cmd == "help" || cmd == "?") {
        return cmd_help();
    }
    if (cmd == "status") {
        return cmd_status();
    }
    if (cmd == "stats") {
        return cmd_stats();
    }
    if (cmd == "config") {
        return cmd_config();
    }
    if (cmd == "routes") {
        return cmd_routes();
    }
    if (cmd == "log") {
        return cmd_log_level(words);
    }
    if (cmd == "ping") {
        return cmd_ping();
    }

    return {"error: unknown command '" + cmd + "'. type 'help' for commands."};
}

std::vector<std::string> CliDispatcher::cmd_help() const {
    return {
        "nvcomm debug cli commands:",
        "  help                 show this help",
        "  status               process/runtime status",
        "  stats                request counters",
        "  config               effective configuration",
        "  routes               registered gateway routes",
        "  log level            show current log level",
        "  log level <name>     set log level (trace|debug|info|warn|error|fatal)",
        "  ping                 connectivity check",
    };
}

std::vector<std::string> CliDispatcher::cmd_status() const {
    if (!context_.config || !context_.running) {
        return {"error: cli context unavailable"};
    }

    return {
        "service=nvcommd",
        "running=" + std::string(context_.running->load() ? "true" : "false"),
        "uptime=" + format_uptime(context_.started_at),
        "config=" + context_.config_path,
        "http=" + context_.config->listen_address + ":" + std::to_string(context_.config->listen_port),
        "unix_socket=" + (context_.config->unix_socket.empty() ? "-" : context_.config->unix_socket),
        "cli_telnet=" + context_.config->cli_listen + ":" + std::to_string(context_.config->cli_port),
    };
}

std::vector<std::string> CliDispatcher::cmd_stats() const {
    if (!context_.stats) {
        return {"error: stats unavailable"};
    }

    return {
        "total_requests=" + std::to_string(context_.stats->total_requests.load()),
        "success_responses=" + std::to_string(context_.stats->success_responses.load()),
        "error_responses=" + std::to_string(context_.stats->error_responses.load()),
        "rate_limited=" + std::to_string(context_.stats->rate_limited.load()),
        "unauthorized=" + std::to_string(context_.stats->unauthorized.load()),
        "cli_commands=" + std::to_string(context_.stats->cli_commands.load()),
    };
}

std::vector<std::string> CliDispatcher::cmd_config() const {
    if (!context_.config) {
        return {"error: config unavailable"};
    }

    const auto& cfg = *context_.config;
    return {
        "server.listen=" + cfg.listen_address,
        "server.port=" + std::to_string(cfg.listen_port),
        "server.threads=" + std::to_string(cfg.io_threads),
        "server.worker_threads=" + std::to_string(cfg.worker_threads),
        "server.max_body_kb=" + std::to_string(cfg.max_body_kb),
        "gateway.rate_limit_rps=" + std::to_string(cfg.gateway_rate_limit_rps),
        "gateway.api_key=" + std::string(cfg.gateway_api_key.empty() ? "(disabled)" : "(set)"),
        "log.level=" + cfg.log_level,
        "log.dir=" + cfg.log_dir,
        "data.persist=" + std::string(cfg.persist ? "true" : "false"),
        "data.event_dir=" + cfg.event_dir,
    };
}

std::vector<std::string> CliDispatcher::cmd_routes() const {
    if (!context_.gateway) {
        return {"error: gateway not available"};
    }
    return context_.gateway->describe_routes();
}

std::vector<std::string> CliDispatcher::cmd_log_level(const std::vector<std::string>& args) const {
    if (args.size() == 1 || (args.size() == 2 && args[1] == "level")) {
        return {"log.level=" + NV_NS_CORE::current_log_level_name()};
    }

    if (args.size() >= 3 && args[1] == "level") {
        NV_NS_CORE::set_log_level(args[2]);
        BOOSTAPP_LOG(Info, "cli set log level to " + args[2]);
        return {"log.level=" + NV_NS_CORE::current_log_level_name()};
    }

    return {"error: usage: log level [trace|debug|info|warn|error|fatal]"};
}

std::vector<std::string> CliDispatcher::cmd_ping() const {
    return {"pong"};
}

NV_NS_INFRA_CLI_END
