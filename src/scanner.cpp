#include "../include/scanner.hpp"
#include "../include/utils.hpp"
#include "../include/icmp.hpp"
#include "../include/tcp.hpp"
#include "../include/thread_pool.hpp"

#include <atomic>
#include <iostream>
#include <mutex>

Scanner::Scanner(const size_t threads, std::string mode, int port)
    : threadCount(threads), mode(std::move(mode)), port(port) {
}

void Scanner::run(const std::string &cidr) const {
    auto [startIp, endIp] = Utils::parseCIDR(cidr);
    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;  // Changed from int to uint32_t
    const uint32_t total = endIp - startIp + 1;  // Changed from int to uint32_t

    std::mutex outputMutex;

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, total, &outputMutex, this] {
            const std::string ipStr = Utils::uintToIp(ip);
            bool isAlive = false;

            if (mode == "icmp") {
                isAlive = Icmp::ping(ipStr);
            } else if (mode == "tcp") {
                isAlive = Tcp::ping(ipStr, port);
            } else if (mode == "fallback") {
                isAlive = Icmp::ping(ipStr) || Tcp::ping(ipStr, port);
            }

            const uint32_t done = ++counter;  // Changed from int to uint32_t
            std::lock_guard<std::mutex> lock(outputMutex);

            // First clear the current line
            std::cerr << "\r";

            // If we found a live IP, print it first
            if (isAlive) {
                std::cerr << ipStr << std::endl;
            }

            // Then print the progress bar
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);  // Added explicit cast

            std::cerr << "[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;
        });
    }

    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "\nScan completed.\n";
}