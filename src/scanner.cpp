#include "../include/scanner.hpp"
#include "../include/utils.hpp"
#include "../include/icmp.hpp"
#include "../include/tcp.hpp"
#include "../include/thread_pool.hpp"
#include "../include/signal_handler.hpp"
#include "../include/logger.hpp"

#include <atomic>
#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

Scanner::Scanner(const size_t threads, std::string mode, int port, int timeoutMs)
    : threadCount(threads ? threads : std::thread::hardware_concurrency()),
      mode(std::move(mode)),
      port(port),
      timeoutMs(timeoutMs) {
}

void Scanner::run(const std::string &cidr) const {
    auto [fst, snd] = Utils::parseCIDR(cidr);
    uint32_t startIp = fst;
    uint32_t endIp = snd;
    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    std::mutex resultsMutex;
    std::vector<std::string> discoveredIps;

    std::atomic<bool> scanComplete = false;
    std::thread progressThread([&counter, total, &scanComplete]() {
        while (!scanComplete && !SignalHandler::isInterrupted()) {
            const uint32_t done = counter.load();
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);

            std::cerr << "\r[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, &resultsMutex, &discoveredIps, this] {
            if (SignalHandler::isInterrupted()) { ++counter; return; }

            const std::string ipStr = Utils::uintToIp(ip);
            bool isAlive = false;

            if (mode == "icmp") {
                isAlive = Icmp::ping(ipStr, true, timeoutMs);
            } else if (mode == "tcp") {
                isAlive = Tcp::ping(ipStr, port, true, timeoutMs);
            } else if (mode == "fallback") {
                isAlive = Icmp::ping(ipStr, true, timeoutMs) || Tcp::ping(ipStr, port, true, timeoutMs);
            }

            ++counter;

            if (isAlive) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                discoveredIps.push_back(ipStr);
            }
        });
    }

    while (counter < total && !SignalHandler::isInterrupted()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (SignalHandler::isInterrupted()) {
        pool.shutdown();
    }

    scanComplete = true;
    progressThread.join();

    std::cerr << "\r" << std::string(80, ' ') << "\r";

    std::sort(discoveredIps.begin(), discoveredIps.end(), [](const std::string& a, const std::string& b) {
        return Utils::ipToUint(a) < Utils::ipToUint(b);
    });

    discoveredIps.erase(
        std::remove_if(discoveredIps.begin(), discoveredIps.end(),
            [startIp, endIp](const std::string& ip) {
                uint32_t ipInt = Utils::ipToUint(ip);
                return (ipInt == startIp || ipInt == endIp);
            }),
        discoveredIps.end()
    );

    std::cout << "Discovered " << discoveredIps.size() << " live hosts:" << std::endl;
    for (const auto& ip : discoveredIps) {
        std::cout << ip << std::endl;
    }

    std::cerr << "Scan completed." << std::endl;
}

NetworkScanner::NetworkScanner(const size_t threads, std::string mode, int port, int timeoutMs)
    : Scanner(threads, std::move(mode), port, timeoutMs) {
}

std::vector<std::string> NetworkScanner::scan(const std::string &cidr) const {
    auto result = Utils::parseCIDR(cidr);
    uint32_t startIp = result.first;
    uint32_t endIp = result.second;

    Logger::verbose("Starting scan of " + cidr + " (" + std::to_string(endIp - startIp + 1) + " hosts)");

    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    std::mutex resultsMutex;
    std::vector<std::string> discoveredIps;

    std::atomic<bool> scanComplete = false;
    std::thread progressThread([&counter, total, &scanComplete]() {
        while (!scanComplete && !SignalHandler::isInterrupted()) {
            const uint32_t done = counter.load();
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);

            std::cerr << "\r[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, &resultsMutex, &discoveredIps, this] {
            if (SignalHandler::isInterrupted()) { ++counter; return; }

            const std::string ipStr = Utils::uintToIp(ip);
            bool isAlive = false;

            if (mode == "icmp") {
                isAlive = Icmp::ping(ipStr, true, timeoutMs);
            } else if (mode == "tcp") {
                isAlive = Tcp::ping(ipStr, port, true, timeoutMs);
            } else if (mode == "fallback") {
                isAlive = Icmp::ping(ipStr, true, timeoutMs) || Tcp::ping(ipStr, port, true, timeoutMs);
            }

            ++counter;

            if (isAlive) {
                Logger::debug("Host alive: " + ipStr);
                std::lock_guard<std::mutex> lock(resultsMutex);
                discoveredIps.push_back(ipStr);
            }
        });
    }

    while (counter < total && !SignalHandler::isInterrupted()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (SignalHandler::isInterrupted()) {
        pool.shutdown();
        Logger::verbose("Scan interrupted by user");
    }

    scanComplete = true;
    progressThread.join();

    std::cerr << "\r" << std::string(80, ' ') << "\r";

    std::sort(discoveredIps.begin(), discoveredIps.end(), [](const std::string& a, const std::string& b) {
        return Utils::ipToUint(a) < Utils::ipToUint(b);
    });

    discoveredIps.erase(
        std::remove_if(discoveredIps.begin(), discoveredIps.end(),
            [startIp, endIp](const std::string& ip) {
                uint32_t ipInt = Utils::ipToUint(ip);
                return (ipInt == startIp || ipInt == endIp);
            }),
        discoveredIps.end()
    );

    Logger::verbose("Scan complete: " + std::to_string(discoveredIps.size()) + " hosts found");
    return discoveredIps;
}

bool NetworkScanner::verifyHost(const std::string& ip) const {
    int successCount = 0;

    if (Icmp::ping(ip, true, timeoutMs)) successCount++;
    if (Tcp::ping(ip, 80, true, timeoutMs)) successCount++;
    if (Tcp::ping(ip, 443, true, timeoutMs)) successCount++;
    if (Tcp::ping(ip, 22, true, timeoutMs)) successCount++;

    return successCount >= 2;
}

std::vector<std::string> NetworkScanner::thoroughScan(const std::string& cidr) const {
    auto [fst, snd] = Utils::parseCIDR(cidr);
    uint32_t startIp = fst;
    uint32_t endIp = snd;

    Logger::verbose("Starting thorough scan of " + cidr);

    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    std::mutex resultsMutex;
    std::vector<std::string> discoveredIps;

    std::atomic<bool> scanComplete = false;
    std::thread progressThread([&counter, total, &scanComplete]() {
        while (!scanComplete && !SignalHandler::isInterrupted()) {
            const uint32_t done = counter.load();
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);

            std::cerr << "\r[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, &resultsMutex, &discoveredIps, this] {
            if (SignalHandler::isInterrupted()) { ++counter; return; }

            const std::string ipStr = Utils::uintToIp(ip);

            bool isAlive = verifyHost(ipStr);

            ++counter;

            if (isAlive) {
                Logger::debug("Host verified (thorough): " + ipStr);
                std::lock_guard<std::mutex> lock(resultsMutex);
                discoveredIps.push_back(ipStr);
            }
        });
    }

    while (counter < total && !SignalHandler::isInterrupted()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (SignalHandler::isInterrupted()) {
        pool.shutdown();
        Logger::verbose("Thorough scan interrupted by user");
    }

    scanComplete = true;
    progressThread.join();

    std::cerr << "\r" << std::string(80, ' ') << "\r";

    std::sort(discoveredIps.begin(), discoveredIps.end(), [](const std::string& a, const std::string& b) {
        return Utils::ipToUint(a) < Utils::ipToUint(b);
    });

    discoveredIps.erase(
        std::remove_if(discoveredIps.begin(), discoveredIps.end(),
            [startIp, endIp](const std::string& ip) {
                uint32_t ipInt = Utils::ipToUint(ip);
                return (ipInt == startIp || ipInt == endIp);
            }),
        discoveredIps.end()
    );

    Logger::verbose("Thorough scan complete: " + std::to_string(discoveredIps.size()) + " hosts verified");
    return discoveredIps;
}
