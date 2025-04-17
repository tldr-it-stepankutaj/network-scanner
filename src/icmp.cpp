#if defined(__APPLE__)
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <chrono>

namespace Icmp {
    bool ping(const std::string& ip) {
        auto start = std::chrono::steady_clock::now();
        std::string cmd = "ping -c 1 -W 1 " + ip + " > /dev/null 2>&1";
        int ret = std::system(cmd.c_str());
        auto end = std::chrono::steady_clock::now();

        if (ret == 0) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::string color = (ms < 30) ? "\033[32m" : (ms < 100) ? "\033[33m" : "\033[31m";
            std::cout << color << ip << " is alive via ping (macOS subprocess, RTT: " << ms << " ms)\033[0m\n";
            return true;
        }

        return false;
    }
}
#else
#include <linux/icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>
#include <iostream>
#include <cstring>

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

    bool ping(const std::string& ip) {
        int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sockfd < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        char sendbuf[64]{};
        auto* icmp = reinterpret_cast<icmphdr*>(sendbuf);
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

        if (select(sockfd + 1, nullptr, nullptr, nullptr, &timeout) > 0) {
            char recvbuf[1500];
            ssize_t n = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
            if (n > 0) {
                auto end = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                std::string color = (ms < 30) ? "\033[32m" : (ms < 100) ? "\033[33m" : "\033[31m";
                std::cout << color << ip << " is alive via ICMP (RTT: " << ms << " ms)\033[0m\n";
                result = true;
            }
        }

        close(sockfd);
        return result;
    }
}
#endif