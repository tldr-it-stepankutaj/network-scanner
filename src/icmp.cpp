#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <atomic>
#include <mutex>

// ICMP headers
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

// If icmphdr is not fully defined, provide a fallback definition
#ifndef ICMP_ECHO
#define ICMP_ECHO 8
#endif

// Some systems use icmp_hdr instead of icmphdr
#if !defined(HAVE_STRUCT_ICMPHDR) && !defined(__GLIBC__)
struct icmphdr {
    uint8_t type;        /* message type */
    uint8_t code;        /* type sub-code */
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;          /* echo datagram */
        uint32_t gateway;    /* gateway address */
        struct {
            uint16_t unused;
            uint16_t mtu;
        } frag;          /* path mtu discovery */
    } un;
};
#endif

#include "../include/icmp.hpp"

namespace Icmp {
    // Protect console output
    static std::mutex outputMutex;

    uint16_t checksum(void* data, int len) {
        auto* buf = static_cast<uint16_t*>(data);
        uint32_t sum = 0;
        for (; len > 1; len -= 2) sum += *buf++;
        if (len == 1) sum += *reinterpret_cast<uint8_t*>(buf);
        sum = (sum >> 16) + (sum & 0xffff);
        sum += (sum >> 16);
        return static_cast<uint16_t>(~sum);
    }

    bool pingRawSocket(const std::string& ip, bool quiet) {
        const int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sockfd < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        char sendbuf[64]{};
        auto* icmp = reinterpret_cast<struct icmphdr*>(sendbuf);
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->un.echo.id = getpid();
        icmp->un.echo.sequence = 1;
        icmp->checksum = checksum(sendbuf, sizeof(sendbuf));

        if (sendto(sockfd, sendbuf, sizeof(sendbuf), 0,
                   reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(sockfd);
            return false;
                   }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        timeval timeout{1, 0};

        bool result = false;

        if (select(sockfd + 1, &fds, nullptr, nullptr, &timeout) > 0) {
            char recvbuf[1500];
            sockaddr_in from{};
            socklen_t socklen = sizeof(from);

            ssize_t n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0,
                               reinterpret_cast<sockaddr*>(&from), &socklen);

            if (n > 0) {
                // Verify this is a response to our ping
                const auto* ip_header = reinterpret_cast<struct ip*>(recvbuf);
                const int ip_header_len = ip_header->ip_hl << 2;

                // ICMP header follows the IP header

                // Check if this is an ICMP ECHO REPLY (type 0) and if it's for our specific ID
                if (const auto* icmp_reply = reinterpret_cast<struct icmphdr*>(recvbuf + ip_header_len); icmp_reply->type == 0 && icmp_reply->un.echo.id == getpid()) {
                    result = true;
                }
            }
        }

        close(sockfd);
        return result;
    }

    bool pingDatagramSocket(const std::string& ip, bool quiet) {
        const int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
        if (sockfd < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        char sendbuf[64]{};
        auto* icmp = reinterpret_cast<struct icmphdr*>(sendbuf);
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->un.echo.id = getpid();
        icmp->un.echo.sequence = 1;
        icmp->checksum = checksum(sendbuf, sizeof(sendbuf));

        auto start = std::chrono::steady_clock::now();

        if (sendto(sockfd, sendbuf, sizeof(sendbuf), 0,
                   reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(sockfd);
            return false;
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        timeval timeout{1, 0};

        bool result = false;

        if (select(sockfd + 1, &fds, nullptr, nullptr, &timeout) > 0) {
            char recvbuf[1500];
            if (const ssize_t n = recv(sockfd, recvbuf, sizeof(recvbuf), 0); n > 0) {
                result = true;
            }
        }

        close(sockfd);
        return result;
    }

    bool pingFallback(const std::string& ip, bool quiet) {
        if (pingDatagramSocket(ip, quiet)) return true;
        if (pingRawSocket(ip, quiet)) return true;

        const std::string cmd = "fping -c1 -t100 " + ip + " 2>/dev/null 1>/dev/null";
        if (const int ret = std::system(cmd.c_str()); ret == 0) {
            return true;
        }

        return false;
    }

    bool ping(const std::string& ip, std::atomic<int>& counter, const int total, const bool quiet) {
        const bool res = pingFallback(ip, quiet);

        // Only lock for output operations
        {
            std::lock_guard<std::mutex> lock(outputMutex);

            // If we found a live IP, print it first (unless in quiet mode)
            if (res && !quiet) {
                std::cerr << ip << std::endl;
            }

            const int done = ++counter;
            constexpr int width = 30;
            const int filled = static_cast<int>((static_cast<double>(done) * width) / total);

            // Print the progress bar (unless in quiet mode)
            if (!quiet) {
                std::cerr << "\r[";
                for (int i = 0; i < width; ++i) std::cerr << (i < filled ? '#' : '.');
                std::cerr << "] " << done << "/" << total << std::flush;
            }
        }

        return res;
    }

    bool ping(const std::string& ip, bool quiet) {
        std::atomic<int> dummy = 0;
        return ping(ip, dummy, 1, quiet);
    }
}