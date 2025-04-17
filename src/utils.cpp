#include <string>
#include <utility>
#include <cstdint>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/utils.hpp"

namespace Utils {
    uint32_t ipToUint(const std::string& ipStr) {
        in_addr ip{};
        inet_pton(AF_INET, ipStr.c_str(), &ip);
        return ntohl(ip.s_addr);
    }

    std::string uintToIp(uint32_t ip) {
        in_addr addr{};
        addr.s_addr = htonl(ip);
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, str, INET_ADDRSTRLEN);
        return {str};
    }

    std::pair<uint32_t, uint32_t> parseCIDR(const std::string& cidr) {
        auto slash = cidr.find('/');
        std::string ipPart = cidr.substr(0, slash);
        int prefix = std::stoi(cidr.substr(slash + 1));
        uint32_t ip = ipToUint(ipPart);
        uint32_t mask = prefix == 0 ? 0 : (~0U << (32 - prefix));
        uint32_t start = ip & mask;
        uint32_t end = start | ~mask;
        return {start, end};
    }
}