#pragma once
#include <string>
#include <atomic>

namespace Icmp {
    uint16_t checksum(void* data, int len);
    bool pingRawSocket(const std::string& ip, bool quiet = false);
    bool pingDatagramSocket(const std::string& ip, bool quiet = false);
    bool pingFallback(const std::string& ip, bool quiet = false);
    bool ping(const std::string& ip, std::atomic<int>& counter, int total, bool quiet = false);
    bool ping(const std::string& ip, bool quiet = false);
}