#pragma once
#include <atomic>
#include <csignal>

namespace SignalHandler {
    inline std::atomic<bool> interrupted{false};

    inline void handler(int) {
        interrupted.store(true);
    }

    inline void install() {
        struct sigaction sa{};
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
    }

    inline bool isInterrupted() {
        return interrupted.load();
    }
}
