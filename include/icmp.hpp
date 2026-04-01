#pragma once
#include <string>
#include <atomic>

namespace Icmp {
    uint16_t checksum(void* data, int len);
    bool pingRawSocket(const std::string& ip, bool quiet = false, int timeoutMs = 1000);
    bool pingDatagramSocket(const std::string& ip, bool quiet = false, int timeoutMs = 1000);
    bool pingFallback(const std::string& ip, bool quiet = false, int timeoutMs = 1000);
    bool ping(const std::string& ip, std::atomic<int>& counter, int total, bool quiet = false, int timeoutMs = 1000);
    bool ping(const std::string& ip, bool quiet = false, int timeoutMs = 1000);
}
