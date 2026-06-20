#include "app/nvcomm_cli/line_editor.hpp"

#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

namespace nvcomm_cli {
namespace {

#if defined(__unix__) || defined(__APPLE__)

class TermiosGuard {
public:
    explicit TermiosGuard(int fd) : fd_(fd), active_(isatty(fd) == 1) {
        if (!active_) {
            return;
        }
        if (tcgetattr(fd_, &original_) != 0) {
            active_ = false;
            return;
        }
        termios raw = original_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_iflag &= static_cast<tcflag_t>(~(ICRNL));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(fd_, TCSAFLUSH, &raw) != 0) {
            active_ = false;
        }
    }

    ~TermiosGuard() {
        if (active_) {
            tcsetattr(fd_, TCSAFLUSH, &original_);
        }
    }

    bool active() const { return active_; }

private:
    int fd_;
    termios original_{};
    bool active_;
};

void write_bytes(const char* data, std::size_t size) {
    std::cout.write(data, static_cast<std::streamsize>(size));
    std::cout.flush();
}

void write_str(const std::string& text) {
    write_bytes(text.data(), text.size());
}

void erase_char() {
    write_str("\b \b");
}

std::string current_word(const std::string& line) {
    const auto pos = line.find_last_of(" \t");
    if (pos == std::string::npos) {
        return line;
    }
    return line.substr(pos + 1);
}

std::vector<std::string> matching_completions(const std::string& prefix,
                                              const std::vector<std::string>& completions) {
    std::vector<std::string> matches;
    for (const auto& candidate : completions) {
        if (candidate.size() >= prefix.size() &&
            candidate.compare(0, prefix.size(), prefix) == 0) {
            matches.push_back(candidate);
        }
    }
    return matches;
}

std::string common_prefix(const std::vector<std::string>& matches) {
    if (matches.empty()) {
        return {};
    }
    std::string prefix = matches.front();
    for (std::size_t i = 1; i < matches.size(); ++i) {
        const auto& value = matches[i];
        std::size_t j = 0;
        while (j < prefix.size() && j < value.size() && prefix[j] == value[j]) {
            ++j;
        }
        prefix.resize(j);
        if (prefix.empty()) {
            break;
        }
    }
    return prefix;
}

void redraw_line(const std::string& prompt, const std::string& line) {
    write_str("\r");
    write_str(prompt);
    write_str(line);
    write_str("\033[K");
}

void complete_word(std::string& line, const std::string& prompt,
                   const std::vector<std::string>& completions) {
    const auto word = current_word(line);
    const auto matches = matching_completions(word, completions);
    if (matches.empty()) {
        return;
    }

    const auto prefix = common_prefix(matches);
    if (prefix.size() > word.size()) {
        line += prefix.substr(word.size());
        redraw_line(prompt, line);
        return;
    }

    if (matches.size() == 1) {
        return;
    }

    write_str("\n");
    for (const auto& match : matches) {
        write_str("  ");
        write_str(match);
        write_str("\n");
    }
    redraw_line(prompt, line);
}

void replace_line(std::string& line, const std::string& new_value) {
    while (!line.empty()) {
        line.pop_back();
        erase_char();
    }
    line = new_value;
    write_str(line);
}

bool read_byte(unsigned char& ch) {
    const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
    return n == 1;
}

enum class ArrowKey { Up, Down };

std::optional<ArrowKey> read_arrow_key() {
    unsigned char ch{};
    if (!read_byte(ch)) {
        return std::nullopt;
    }
    if (ch != '[' && ch != 'O') {
        return std::nullopt;
    }
    if (!read_byte(ch)) {
        return std::nullopt;
    }
    if (ch == 'A') {
        return ArrowKey::Up;
    }
    if (ch == 'B') {
        return ArrowKey::Down;
    }
    return std::nullopt;
}

class HistoryBrowser {
public:
    explicit HistoryBrowser(std::vector<std::string>* history) : history_(history) {}

    bool move_up(std::string& line) {
        if (!history_ || history_->empty()) {
            return false;
        }
        if (index_ < 0) {
            draft_ = line;
            index_ = static_cast<int>(history_->size()) - 1;
        } else if (index_ > 0) {
            --index_;
        }
        line = (*history_)[static_cast<std::size_t>(index_)];
        return true;
    }

    bool move_down(std::string& line) {
        if (!history_ || index_ < 0) {
            return false;
        }
        if (index_ + 1 < static_cast<int>(history_->size())) {
            ++index_;
            line = (*history_)[static_cast<std::size_t>(index_)];
        } else {
            index_ = -1;
            line = draft_;
        }
        return true;
    }

    void reset_index() {
        index_ = -1;
    }

    void reset_browse() {
        index_ = -1;
        draft_.clear();
    }

    void commit(const std::string& line) {
        if (!history_ || line.empty()) {
            reset_browse();
            return;
        }
        if (history_->empty() || history_->back() != line) {
            history_->push_back(line);
        }
        reset_browse();
    }

private:
    std::vector<std::string>* history_;
    std::string draft_;
    int index_{-1};
};

#endif

}  // namespace

bool stdin_is_tty() {
#if defined(__unix__) || defined(__APPLE__)
    return isatty(STDIN_FILENO) == 1;
#else
    return false;
#endif
}

std::optional<std::string> read_line(const std::string& prompt,
                                     const std::vector<std::string>& completions,
                                     std::vector<std::string>* history) {
#if defined(__unix__) || defined(__APPLE__)
    if (!stdin_is_tty()) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            return std::nullopt;
        }
        if (history && !line.empty()) {
            if (history->empty() || history->back() != line) {
                history->push_back(line);
            }
        }
        return line;
    }

    TermiosGuard guard(STDIN_FILENO);
    if (!guard.active()) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            return std::nullopt;
        }
        if (history && !line.empty()) {
            if (history->empty() || history->back() != line) {
                history->push_back(line);
            }
        }
        return line;
    }

    HistoryBrowser browser(history);
    std::string line;
    write_str(prompt);

    while (true) {
        unsigned char ch{};
        const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n < 0) {
            write_str("\n");
            return line.empty() ? std::nullopt : std::optional<std::string>{line};
        }
        if (n == 0) {
            write_str("\n");
            return line.empty() ? std::nullopt : std::optional<std::string>{line};
        }

        if (ch == '\n' || ch == '\r') {
            write_str("\n");
            browser.commit(line);
            return line;
        }
        if (ch == 127 || ch == 8) {
            if (!line.empty()) {
                browser.reset_index();
                line.pop_back();
                erase_char();
            }
            continue;
        }
        if (ch == 4) {  // Ctrl-D
            write_str("\n");
            return line.empty() ? std::nullopt : std::optional<std::string>{line};
        }
        if (ch == 3) {  // Ctrl-C
            write_str("^C\n");
            line.clear();
            browser.reset_browse();
            write_str(prompt);
            continue;
        }
        if (ch == '\t') {
            browser.reset_index();
            complete_word(line, prompt, completions);
            continue;
        }
        if (ch == 27) {
            const auto arrow = read_arrow_key();
            if (arrow == ArrowKey::Up) {
                std::string next = line;
                if (browser.move_up(next)) {
                    replace_line(line, next);
                }
                continue;
            }
            if (arrow == ArrowKey::Down) {
                std::string next = line;
                if (browser.move_down(next)) {
                    replace_line(line, next);
                }
                continue;
            }
            continue;
        }
        if (static_cast<unsigned char>(ch) < 32) {
            continue;
        }

        browser.reset_index();
        line.push_back(static_cast<char>(ch));
        write_bytes(reinterpret_cast<const char*>(&ch), 1);
    }
#else
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::nullopt;
    }
    if (history && !line.empty()) {
        if (history->empty() || history->back() != line) {
            history->push_back(line);
        }
    }
    return line;
#endif
}

}  // namespace nvcomm_cli
