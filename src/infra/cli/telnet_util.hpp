#pragma once

#include "namespace.hpp"

#include <cstdint>
#include <string>
#include <vector>

NV_NS_INFRA_CLI_BEGIN

struct TelnetParseResult {
    std::vector<std::uint8_t> replies;
    bool line_ready{false};
    std::string line;
};

// Filters telnet IAC bytes and assembles one input line.
// Does not echo: the telnet client displays typed characters locally.
class TelnetLineReader {
public:
    TelnetParseResult feed(std::uint8_t byte);

private:
    void handle_iac(std::uint8_t cmd, std::uint8_t opt, std::vector<std::uint8_t>& replies);

    enum class State { Normal, Iac, IacCmd, IacSb };

    State state_{State::Normal};
    std::uint8_t iac_cmd_{0};
    std::string line_;
    bool swallow_lf_{false};
};

std::string telnet_negotiation();
std::string encode_telnet_lines(const std::vector<std::string>& lines);
std::string telnet_prompt();

NV_NS_INFRA_CLI_END
