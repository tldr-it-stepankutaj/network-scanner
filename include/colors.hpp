#pragma once
#include <string>
#include <cstdlib>

namespace Colors {
    inline std::string GREEN = "\033[32m";
    inline std::string BRIGHT_GREEN = "\033[92m";
    inline std::string CYAN = "\033[36m";
    inline std::string YELLOW = "\033[33m";
    inline std::string RED = "\033[31m";
    inline std::string RESET = "\033[0m";
    inline std::string BOLD = "\033[1m";

    inline void disable() {
        GREEN = "";
        BRIGHT_GREEN = "";
        CYAN = "";
        YELLOW = "";
        RED = "";
        RESET = "";
        BOLD = "";
    }

    inline bool shouldDisable() {
        return std::getenv("NO_COLOR") != nullptr;
    }
}
