#include "app/nvcomm_cli/line_editor.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace {

const std::vector<std::string> kCliCompletions = {
    "help", "?", "status", "stats", "config", "routes", "log", "ping", "exit", "quit",
    "level", "trace", "debug", "info", "warn", "error", "fatal",
};

}  // namespace

namespace asio = boost::asio;

std::string read_response(asio::local::stream_protocol::socket& socket) {
    asio::streambuf buffer;
    boost::system::error_code ec;
    asio::read_until(socket, buffer, ".\n", ec);
    if (ec) {
        throw std::runtime_error("read failed: " + ec.message());
    }

    std::istream input(&buffer);
    std::string line;
    std::string body;
    while (std::getline(input, line)) {
        if (line == ".") {
            break;
        }
        if (!body.empty()) {
            body += '\n';
        }
        body += line;
    }
    return body;
}

bool send_command(asio::local::stream_protocol::socket& socket, const std::string& command) {
    std::string line = command;
    if (line.empty() || line.back() != '\n') {
        line += '\n';
    }

    boost::system::error_code ec;
    asio::write(socket, asio::buffer(line), ec);
    if (ec) {
        std::cerr << "write failed: " << ec.message() << '\n';
        return false;
    }

    const auto response = read_response(socket);
    if (!response.empty()) {
        std::cout << response << '\n';
    }
    return true;
}

void print_usage(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [options] [command]\n"
              << "  -s, --socket PATH   cli unix socket (default: run/nvcomm.cli.sock)\n"
              << "  -h, --help          show help\n"
              << "\nExamples:\n"
              << "  " << argv0 << " stats\n"
              << "  " << argv0 << " log level debug\n"
              << "  " << argv0 << "                 # interactive mode\n";
}

int main(int argc, char** argv) {
    std::string socket_path = "run/nvcomm.cli.sock";
    std::string one_shot_command;
    bool interactive = true;

    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        if ((arg == "-s" || arg == "--socket") && i + 1 < argc) {
            socket_path = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-i" || arg == "--interactive") {
            interactive = true;
        } else {
            one_shot_command = arg;
            for (int j = i + 1; j < argc; ++j) {
                one_shot_command += ' ';
                one_shot_command += argv[j];
            }
            interactive = false;
            break;
        }
    }

    try {
        asio::io_context io;
        asio::local::stream_protocol::socket socket(io);
        socket.connect(asio::local::stream_protocol::endpoint(socket_path));

        if (!interactive) {
            return send_command(socket, one_shot_command) ? 0 : 1;
        }

        std::cout << "nvcomm-cli connected to " << socket_path << '\n';
        std::cout << "type 'help' for commands, 'exit' to quit\n";
        std::cout << "up/down arrows browse command history\n";

        std::vector<std::string> history;
        while (true) {
            auto line = nvcomm_cli::read_line("nvcomm> ", kCliCompletions, &history);
            if (!line) {
                break;
            }
            if (*line == "exit" || *line == "quit") {
                break;
            }
            if (line->empty()) {
                continue;
            }
            if (!send_command(socket, *line)) {
                return 1;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "cli error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
