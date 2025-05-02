#include "../include/scanner.hpp"
#include "../include/utils.hpp"
#include "../include/icmp.hpp"
#include "../include/tcp.hpp"
#include "../include/thread_pool.hpp"

#include <atomic>
#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

Scanner::Scanner(const size_t threads, std::string mode, int port)
    : threadCount(threads ? threads : std::thread::hardware_concurrency()),
      mode(std::move(mode)),
      port(port) {
}

void Scanner::run(const std::string &cidr) const {
    auto [fst, snd] = Utils::parseCIDR(cidr);
    uint32_t startIp = fst;
    uint32_t endIp = snd;
    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    // Vector to collect discovered IPs
    std::mutex resultsMutex;
    std::vector<std::string> discoveredIps;

    // Create a separate thread for updating the progress bar
    std::atomic<bool> scanComplete = false;
    std::thread progressThread([&counter, total, &scanComplete]() {
        while (!scanComplete) {
            const uint32_t done = counter.load();
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);

            std::cerr << "\r[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            // Update progress every 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Schedule the scan tasks
    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, &resultsMutex, &discoveredIps, this] {
            const std::string ipStr = Utils::uintToIp(ip);
            bool isAlive = false;

            if (mode == "icmp") {
                isAlive = Icmp::ping(ipStr, true); // Set quiet mode to true
            } else if (mode == "tcp") {
                isAlive = Tcp::ping(ipStr, port, true); // Set quiet mode to true
            } else if (mode == "fallback") {
                isAlive = Icmp::ping(ipStr, true) || Tcp::ping(ipStr, port, true);
            }

            // Increment the counter regardless of result
            ++counter;

            // If the IP is alive, add it to our results vector
            if (isAlive) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                discoveredIps.push_back(ipStr);
            }
        });
    }

    // Wait for all tasks to complete
    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Signal the progress thread to exit and wait for it
    scanComplete = true;
    progressThread.join();

    // Clear the progress bar line
    std::cerr << "\r" << std::string(80, ' ') << "\r";

    // Sort IPs in ascending order
    std::sort(discoveredIps.begin(), discoveredIps.end(), [](const std::string& a, const std::string& b) {
        return Utils::ipToUint(a) < Utils::ipToUint(b);
    });

    // Filter out special network addresses
    discoveredIps.erase(
        std::remove_if(discoveredIps.begin(), discoveredIps.end(),
            [startIp, endIp](const std::string& ip) {
                uint32_t ipInt = Utils::ipToUint(ip);
                return (ipInt == startIp || ipInt == endIp);
            }),
        discoveredIps.end()
    );

    // Now print all discovered IPs
    std::cout << "Discovered " << discoveredIps.size() << " live hosts:" << std::endl;
    for (const auto& ip : discoveredIps) {
        std::cout << ip << std::endl;
    }

    std::cerr << "Scan completed." << std::endl;
}

// NetworkScanner implementation
NetworkScanner::NetworkScanner(const size_t threads, std::string mode, int port)
    : Scanner(threads, std::move(mode), port) {
}

std::vector<std::string> NetworkScanner::scan(const std::string &cidr) const {
    auto result = Utils::parseCIDR(cidr);
    uint32_t startIp = result.first;
    uint32_t endIp = result.second;
    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    // Vector to collect discovered IPs
    std::mutex resultsMutex;
    std::vector<std::string> discoveredIps;

    // Create a separate thread for updating the progress bar
    std::atomic<bool> scanComplete = false;
    std::thread progressThread([&counter, total, &scanComplete]() {
        while (!scanComplete) {
            const uint32_t done = counter.load();
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);

            std::cerr << "\r[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            // Update progress every 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Schedule the scan tasks
    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, &resultsMutex, &discoveredIps, this] {
            const std::string ipStr = Utils::uintToIp(ip);
            bool isAlive = false;

            if (mode == "icmp") {
                isAlive = Icmp::ping(ipStr, true); // Set quiet mode to true
            } else if (mode == "tcp") {
                isAlive = Tcp::ping(ipStr, port, true); // Set quiet mode to true
            } else if (mode == "fallback") {
                isAlive = Icmp::ping(ipStr, true) || Tcp::ping(ipStr, port, true);
            }

            // Increment the counter regardless of result
            ++counter;

            // If the IP is alive, add it to our results vector
            if (isAlive) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                discoveredIps.push_back(ipStr);
            }
        });
    }

    // Wait for all tasks to complete
    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Signal the progress thread to exit and wait for it
    scanComplete = true;
    progressThread.join();

    // Clear the progress bar line
    std::cerr << "\r" << std::string(80, ' ') << "\r";

    // Sort IPs in ascending order
    std::sort(discoveredIps.begin(), discoveredIps.end(), [](const std::string& a, const std::string& b) {
        return Utils::ipToUint(a) < Utils::ipToUint(b);
    });

    // Filter out special network addresses
    discoveredIps.erase(
        std::remove_if(discoveredIps.begin(), discoveredIps.end(),
            [startIp, endIp](const std::string& ip) {
                uint32_t ipInt = Utils::ipToUint(ip);
                return (ipInt == startIp || ipInt == endIp);
            }),
        discoveredIps.end()
    );

    return discoveredIps;
}

bool NetworkScanner::verifyHost(const std::string& ip) {
    // Implementation of a more thorough host verification
    // Require at least 2 different detection methods to succeed

    int successCount = 0;

    // Try ICMP ping
    if (Icmp::ping(ip, true)) {
        successCount++;
    }

    // Try TCP on port 80
    if (Tcp::ping(ip, 80, true)) {
        successCount++;
    }

    // Try TCP on port 443 (HTTPS)
    if (Tcp::ping(ip, 443, true)) {
        successCount++;
    }

    // Try TCP on port 22 (SSH)
    if (Tcp::ping(ip, 22, true)) {
        successCount++;
    }

    // Consider the host verified if at least 2 methods succeed
    return successCount >= 2;
}

std::vector<std::string> NetworkScanner::thoroughScan(const std::string& cidr) const {
    auto [fst, snd] = Utils::parseCIDR(cidr);
    uint32_t startIp = fst;
    uint32_t endIp = snd;
    ThreadPool pool(threadCount);

    std::atomic<uint32_t> counter = 0;
    const uint32_t total = endIp - startIp + 1;

    // Vector to collect discovered IPs
    std::mutex resultsMutex;
    std::vector<std::string> discoveredIps;

    // Create a separate thread for updating the progress bar
    std::atomic<bool> scanComplete = false;
    std::thread progressThread([&counter, total, &scanComplete]() {
        while (!scanComplete) {
            const uint32_t done = counter.load();
            constexpr int width = 30;
            const int filled = static_cast<int>((done * width) / total);

            std::cerr << "\r[";
            for (int i = 0; i < width; ++i)
                std::cerr << (i < filled ? '#' : '.');
            std::cerr << "] " << done << "/" << total << std::flush;

            // Update progress every 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Schedule the scan tasks
    for (uint32_t ip = startIp; ip <= endIp; ++ip) {
        pool.enqueue([ip, &counter, &resultsMutex, &discoveredIps, this] {
            const std::string ipStr = Utils::uintToIp(ip);

            // Use the more thorough verification method
            bool isAlive = verifyHost(ipStr);

            // Increment the counter regardless of result
            ++counter;

            // If the IP is alive, add it to our results vector
            if (isAlive) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                discoveredIps.push_back(ipStr);
            }
        });
    }

    // Wait for all tasks to complete
    while (counter < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Signal the progress thread to exit and wait for it
    scanComplete = true;
    progressThread.join();

    // Clear the progress bar line
    std::cerr << "\r" << std::string(80, ' ') << "\r";

    // Sort IPs in ascending order
    std::sort(discoveredIps.begin(), discoveredIps.end(), [](const std::string& a, const std::string& b) {
        return Utils::ipToUint(a) < Utils::ipToUint(b);
    });

    // Filter out special network addresses
    discoveredIps.erase(
        std::remove_if(discoveredIps.begin(), discoveredIps.end(),
            [startIp, endIp](const std::string& ip) {
                uint32_t ipInt = Utils::ipToUint(ip);
                return (ipInt == startIp || ipInt == endIp);
            }),
        discoveredIps.end()
    );

    return discoveredIps;
}