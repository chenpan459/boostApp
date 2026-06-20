#pragma once

#include "infra/cli/cli_context.hpp"
#include "namespace.hpp"

#include <string>
#include <vector>

NV_NS_INFRA_CLI_BEGIN

class CliDispatcher {
public:
    explicit CliDispatcher(CliContext context);

    std::vector<std::string> dispatch(std::string line);

private:
    std::vector<std::string> cmd_help() const;
    std::vector<std::string> cmd_status() const;
    std::vector<std::string> cmd_stats() const;
    std::vector<std::string> cmd_config() const;
    std::vector<std::string> cmd_routes() const;
    std::vector<std::string> cmd_log_level(const std::vector<std::string>& args) const;
    std::vector<std::string> cmd_ping() const;

    CliContext context_;
};

NV_NS_INFRA_CLI_END
