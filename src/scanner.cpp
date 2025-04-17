#include "../include/scanner.hpp"
#include "../include/utils.hpp"
#include "../include/icmp.hpp"
#include "../include/tcp.hpp"
#include "../include/thread_pool.hpp"
#include <atomic>
#include <iostream>

Scanner::Scanner(size_t threads, std::string mode, int port)
    : threadCount(threads), mode(std::move(mode)), port(port) {
}

void Scanner::run(const std::string &cidr) const {
    auto [startIp, endIp] = Utils::parseCIDR(cidr);
    ThreadPool pool(threadCount);
    std::atomic<int> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, total, this] {
            const std::string ipStr = Utils::uintToIp(ip);

            if (mode == "icmp") {
                if (Icmp::ping(ipStr, counter, total)) {
                    std::cout << "\n" << ipStr << "\n";
                }
            } else if (mode == "tcp") {
                Tcp::ping(ipStr, port);
            } else if (mode == "fallback") {
                if (!Icmp::ping(ipStr, counter, total)) {
                    Tcp::ping(ipStr, port);
                } else {
                    std::cout << "\n" << ipStr << "\n";
                }
            }
        });
    }

    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cerr << "\nScan completed.\n";
}
