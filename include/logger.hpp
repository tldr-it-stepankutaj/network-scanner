#pragma once
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace Logger {
    enum class Level { QUIET, NORMAL, VERBOSE, DEBUG };

    inline Level currentLevel = Level::NORMAL;
    inline std::mutex logMutex;

    inline void setLevel(Level level) { currentLevel = level; }

    inline std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    inline void debug(const std::string& msg) {
        if (currentLevel >= Level::DEBUG) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cerr << "[" << timestamp() << " DBG] " << msg << std::endl;
        }
    }

    inline void verbose(const std::string& msg) {
        if (currentLevel >= Level::VERBOSE) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cerr << "[" << timestamp() << " INF] " << msg << std::endl;
        }
    }

    inline void warn(const std::string& msg) {
        if (currentLevel >= Level::NORMAL) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cerr << "[" << timestamp() << " WRN] " << msg << std::endl;
        }
    }

    inline void error(const std::string& msg) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "[" << timestamp() << " ERR] " << msg << std::endl;
    }
}
