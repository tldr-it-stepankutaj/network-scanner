#pragma once
#include <string>
#include <atomic>

namespace Icmp {
    bool ping(const std::string& ip, bool quiet = false);
    bool ping(const std::string& ip, std::atomic<int>& counter, int total, bool quiet = false);
}