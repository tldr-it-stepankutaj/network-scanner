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
#include <netinet/ip_icmp.h>
#include "../include/icmp.hpp"

namespace Icmp {
    uint16_t checksum(void* data, int len) {
        auto* buf = static_cast<uint16_t*>(data);
        uint32_t sum = 0;
        for (; len > 1; len -= 2) sum += *buf++;
        if (len == 1) sum += *reinterpret_cast<uint8_t*>(buf);
        sum = (sum >> 16) + (sum & 0xffff);
        sum += (sum >> 16);
        return static_cast<uint16_t>(~sum);
    }

    bool pingRawSocket(const std::string& ip) {
        int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sockfd < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        char sendbuf[64]{};
        auto* icmp = reinterpret_cast<struct icmphdr*>(sendbuf);
        icmp->type = 8; // ICMP_ECHO
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
            ssize_t n = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
            if (n > 0) {
                std::cout << ip << "\n";
                result = true;
            }
        }

        close(sockfd);
        return result;
    }

    bool pingDatagramSocket(const std::string& ip) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
        if (sockfd < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        char sendbuf[64]{};
        auto* icmp = reinterpret_cast<struct icmphdr*>(sendbuf);
        icmp->type = 8; // ICMP_ECHO
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
            ssize_t n = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
            if (n > 0) {
                std::cout << ip << "\n";
                result = true;
            }
        }

        close(sockfd);
        return result;
    }

    bool pingFallback(const std::string& ip) {
        if (pingDatagramSocket(ip)) return true;
        if (pingRawSocket(ip)) return true;

        std::string cmd = "fping -c1 -t100 " + ip + " 2>/dev/null 1>/dev/null";
        int ret = std::system(cmd.c_str());
        if (ret == 0) {
            std::cout << ip << "\n";
            return true;
        }

        return false;
    }

    bool ping(const std::string& ip, std::atomic<int>& counter, int total) {
        const bool res = pingFallback(ip);
        const int done = ++counter;
        const int width = 30;
        const int filled = (done * width) / total;
        std::cout << "\r[";
        for (int i = 0; i < width; ++i) std::cout << (i < filled ? '#' : '.');
        std::cout << "] " << done << "/" << total << std::flush;
        return res;
    }

    bool ping(const std::string& ip) {
        std::atomic<int> dummy = 0;
        return ping(ip, dummy, 1);
    }
}
