#pragma once

#include "infra/cli/cli_context.hpp"
#include "infra/cli/cli_dispatcher.hpp"
#include "namespace.hpp"

#include <boost/asio/io_context.hpp>

#include <memory>
#include <string>

NV_NS_INFRA_CLI_BEGIN

class CliServer {
public:
    CliServer(boost::asio::io_context& io, CliContext context);

    void start();
    void stop();

private:
    class Impl;
    std::string socket_path_;
    CliDispatcher dispatcher_;
    std::shared_ptr<Impl> impl_;
};

NV_NS_INFRA_CLI_END
