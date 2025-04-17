#include "../include/scanner.hpp"
#include "../include/utils.hpp"
#include "../include/icmp.hpp"
#include "../include/tcp.hpp"
#include "../include/thread_pool.hpp"

#include <atomic>
#include <iostream>
#include <mutex>

Scanner::Scanner(size_t threads, std::string mode, int port)
    : threadCount(threads), mode(std::move(mode)), port(port) {}

void Scanner::run(const std::string& cidr) const {
    auto [startIp, endIp] = Utils::parseCIDR(cidr);
    ThreadPool pool(threadCount);

    std::atomic<int> counter = 0;
    const int total = endIp - startIp + 1;

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

            int done = ++counter;

            {
                std::lock_guard<std::mutex> lock(outputMutex);

                // Progress bar
                const int width = 30;
                int filled = (done * width) / total;

                std::cerr << "\r[";
                for (int i = 0; i < width; ++i)
                    std::cerr << (i < filled ? '#' : '.');
                std::cerr << "] " << done << "/" << total << std::flush;

                // Print only successful IPs
                if (isAlive) {
                    std::cout << "\n" << ipStr << std::flush;
                }
            }
        });
    }

    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "\nScan completed.\n";
}