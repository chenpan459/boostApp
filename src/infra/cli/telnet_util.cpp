#include "infra/cli/telnet_util.hpp"

NV_NS_INFRA_CLI_BEGIN
namespace {

constexpr std::uint8_t kIac = 255;
constexpr std::uint8_t kWill = 251;
constexpr std::uint8_t kWont = 252;
constexpr std::uint8_t kDo = 253;
constexpr std::uint8_t kDont = 254;

void push_iac(std::vector<std::uint8_t>& out, std::uint8_t cmd, std::uint8_t opt) {
    out.push_back(kIac);
    out.push_back(cmd);
    out.push_back(opt);
}

}  // namespace

std::string telnet_negotiation() {
    // Let telnet client echo and edit locally (standard inetutils telnet behavior).
    std::string out;
    out.push_back(static_cast<char>(kIac));
    out.push_back(static_cast<char>(kWont));
    out.push_back(static_cast<char>(1));  // ECHO
    out.push_back(static_cast<char>(kIac));
    out.push_back(static_cast<char>(kDo));
    out.push_back(static_cast<char>(3));  // SGA
    return out;
}

TelnetParseResult TelnetLineReader::feed(std::uint8_t byte) {
    TelnetParseResult result;

    if (state_ == State::Iac) {
        if (byte == kIac) {
            line_.push_back(static_cast<char>(byte));
            state_ = State::Normal;
            return result;
        }
        if (byte == 250) {
            state_ = State::IacSb;
            return result;
        }
        iac_cmd_ = byte;
        state_ = State::IacCmd;
        return result;
    }

    if (state_ == State::IacCmd) {
        handle_iac(iac_cmd_, byte, result.replies);
        state_ = State::Normal;
        return result;
    }

    if (state_ == State::IacSb) {
        if (byte == kIac) {
            state_ = State::Iac;
        }
        return result;
    }

    if (byte == kIac) {
        state_ = State::Iac;
        return result;
    }

    if (byte == '\r') {
        result.line_ready = true;
        result.line = std::move(line_);
        line_.clear();
        swallow_lf_ = true;
        return result;
    }

    if (byte == '\n') {
        if (swallow_lf_) {
            swallow_lf_ = false;
            return result;
        }
        result.line_ready = true;
        result.line = std::move(line_);
        line_.clear();
        return result;
    }

    if (byte == 4) {  // Ctrl-D
        result.line_ready = true;
        result.line = "exit";
        line_.clear();
        swallow_lf_ = false;
        return result;
    }

    if (byte == 127 || byte == 8 || byte < 32) {
        return result;
    }

    line_.push_back(static_cast<char>(byte));
    return result;
}

void TelnetLineReader::handle_iac(std::uint8_t cmd, std::uint8_t opt, std::vector<std::uint8_t>& replies) {
    if (cmd == kDo) {
        if (opt == 1) {
            push_iac(replies, kWont, opt);
        } else if (opt == 3) {
            push_iac(replies, kWill, opt);
        } else {
            push_iac(replies, kWont, opt);
        }
        return;
    }
    if (cmd == kWill) {
        if (opt == 1) {
            push_iac(replies, kDo, opt);
        } else if (opt == 3) {
            push_iac(replies, kDo, opt);
        } else {
            push_iac(replies, kDont, opt);
        }
        return;
    }
}

std::string encode_telnet_lines(const std::vector<std::string>& lines) {
    std::string out;
    for (const auto& line : lines) {
        out += line;
        out += "\r\n";
    }
    return out;
}

std::string telnet_prompt() {
    return "nvcomm> ";
}

NV_NS_INFRA_CLI_END
