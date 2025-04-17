#include "scanner.hpp"
#include "utils.hpp"
#include "icmp.hpp"
#include "tcp.hpp"
#include "thread_pool.hpp"
#include <atomic>
#include <iostream>

Scanner::Scanner(size_t threads, std::string mode, int port)
    : threadCount(threads), mode(std::move(mode)), port(port) {}

void Scanner::run(const std::string& cidr) const {
    auto [startIp, endIp] = Utils::parseCIDR(cidr);
    ThreadPool pool(threadCount);
    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, total, this] {
            const std::string ipStr = Utils::uintToIp(ip);

            if (mode == "icmp") {
                Icmp::ping(ipStr);
            } else if (mode == "tcp") {
                Tcp::ping(ipStr, port);
            } else if (mode == "fallback") {
                if (!Icmp::ping(ipStr)) {
                    Tcp::ping(ipStr, port);
                }
            }

            if (++counter % 1000 == 0) {
                std::cerr << counter << " / " << total << " done\r";
            }
        });
    }

    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cerr << "\nScan completed.\n";
}