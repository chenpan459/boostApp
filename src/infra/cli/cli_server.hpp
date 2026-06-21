#pragma once

#include "infra/cli/cli_context.hpp"
#include "infra/cli/cli_dispatcher.hpp"
#include "namespace.hpp"

#include <boost/asio/io_context.hpp>

#include <cstdint>
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
    std::string listen_address_;
    std::uint16_t listen_port_{0};
    CliDispatcher dispatcher_;
    std::shared_ptr<Impl> impl_;
};

NV_NS_INFRA_CLI_END
