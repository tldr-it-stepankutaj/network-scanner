#include "../include/scanner.hpp"
#include "../include/utils.hpp"
#include "../include/icmp.hpp"
#include "../include/tcp.hpp"
#include "../include/thread_pool.hpp"
#include <atomic>
#include <iostream>
#include <mutex>

Scanner::Scanner(size_t threads, std::string mode, int port)
    : threadCount(threads), mode(std::move(mode)), port(port) {
}

void Scanner::run(const std::string &cidr) const {
    auto [startIp, endIp] = Utils::parseCIDR(cidr);
    ThreadPool pool(threadCount);
    std::atomic<int> counter = 0;
    std::mutex printMutex;
    const uint32_t total = endIp - startIp + 1;

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, total, &printMutex, this] {
            const std::string ipStr = Utils::uintToIp(ip);

            bool isAlive = false;

            if (mode == "icmp") {
                isAlive = Icmp::ping(ipStr);
            } else if (mode == "tcp") {
                isAlive = Tcp::ping(ipStr, port);
            } else if (mode == "fallback") {
                isAlive = Icmp::ping(ipStr);
                if (!isAlive) {
                    isAlive = Tcp::ping(ipStr, port);
                }
            }

            int done = ++counter;

            // LOCK: tisk pouze 1 threadem najednou
            std::lock_guard<std::mutex> lock(printMutex);

            std::cerr << "\r[";
            const int width = 30;
            int filled = (done * width) / total;
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            if (isAlive) {
                std::cout << ipStr << "\n";
            }
        });
    }

    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cerr << "\nScan completed.\n";
}
