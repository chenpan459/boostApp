#pragma once

#include <optional>
#include <string>
#include <vector>

namespace nvcomm_cli {

bool stdin_is_tty();

// Reads one edited line from stdin when attached to a TTY.
// Returns nullopt on EOF (Ctrl-D). Falls back to std::getline when not a TTY.
// history stores previous commands; up/down arrows navigate it.
std::optional<std::string> read_line(const std::string& prompt,
                                     const std::vector<std::string>& completions,
                                     std::vector<std::string>* history = nullptr);

}  // namespace nvcomm_cli
