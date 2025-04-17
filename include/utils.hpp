#pragma once
#include <string>
#include <utility>

namespace Utils {
    uint32_t ipToUint(const std::string& ipStr);
    std::string uintToIp(uint32_t ip);
    std::pair<uint32_t, uint32_t> parseCIDR(const std::string& cidr);
}