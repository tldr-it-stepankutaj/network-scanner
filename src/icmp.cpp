#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <chrono>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <atomic>
#include <mutex>
#include <fcntl.h>

// ICMP headers
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#ifndef ICMP_ECHO
#define ICMP_ECHO 8
#endif

#if !defined(HAVE_STRUCT_ICMPHDR) && !defined(__GLIBC__)
struct icmphdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
        struct {
            uint16_t unused;
            uint16_t mtu;
        } frag;
    } un;
};
#endif

#include "../include/icmp.hpp"
#include "../include/utils.hpp"
#include "../include/logger.hpp"

namespace Icmp {
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

    bool pingRawSocket(const std::string& ip, bool quiet, int timeoutMs) {
        if (!Utils::isValidIpv4(ip)) {
            Logger::debug("pingRawSocket: invalid IP " + ip);
            return false;
        }

        const int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sockfd < 0) {
            Logger::debug("pingRawSocket: cannot create raw socket (need root?)");
            return false;
        }

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
            Logger::debug("pingRawSocket: sendto failed for " + ip);
            close(sockfd);
            return false;
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        timeval timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1000};

        bool result = false;

        if (select(sockfd + 1, &fds, nullptr, nullptr, &timeout) > 0) {
            char recvbuf[1500];
            sockaddr_in from{};
            socklen_t socklen = sizeof(from);

            ssize_t n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0,
                               reinterpret_cast<sockaddr*>(&from), &socklen);

            if (n > 0) {
                const auto* ip_header = reinterpret_cast<struct ip*>(recvbuf);
                const int ip_header_len = ip_header->ip_hl << 2;

                if (const auto* icmp_reply = reinterpret_cast<struct icmphdr*>(recvbuf + ip_header_len); icmp_reply->type == 0 && icmp_reply->un.echo.id == getpid()) {
                    result = true;
                    Logger::debug("pingRawSocket: " + ip + " is alive");
                }
            }
        }

        close(sockfd);
        return result;
    }

    bool pingDatagramSocket(const std::string& ip, bool quiet, int timeoutMs) {
        if (!Utils::isValidIpv4(ip)) return false;

        const int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
        if (sockfd < 0) {
            Logger::debug("pingDatagramSocket: cannot create dgram ICMP socket");
            return false;
        }

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
        timeval timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1000};

        bool result = false;

        if (select(sockfd + 1, &fds, nullptr, nullptr, &timeout) > 0) {
            char recvbuf[1500];
            if (const ssize_t n = recv(sockfd, recvbuf, sizeof(recvbuf), 0); n > 0) {
                result = true;
                Logger::debug("pingDatagramSocket: " + ip + " is alive");
            }
        }

        close(sockfd);
        return result;
    }

    bool pingFallback(const std::string& ip, bool quiet, int timeoutMs) {
        if (!Utils::isValidIpv4(ip)) {
            Logger::warn("pingFallback: rejecting invalid IP: " + ip);
            return false;
        }

        if (pingDatagramSocket(ip, quiet, timeoutMs)) return true;
        if (pingRawSocket(ip, quiet, timeoutMs)) return true;

        // Use fork/exec instead of system() to avoid shell injection
        Logger::debug("pingFallback: trying fping for " + ip);
        std::string timeoutArg = "-t" + std::to_string(timeoutMs);

        pid_t pid = fork();
        if (pid == 0) {
            // Child process: redirect stdout/stderr to /dev/null
            int devnull = open("/dev/null", O_RDWR);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            execlp("fping", "fping", "-c1", timeoutArg.c_str(), ip.c_str(), nullptr);
            _exit(127);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                Logger::debug("pingFallback: fping reports " + ip + " alive");
                return true;
            }
        }

        return false;
    }

    bool ping(const std::string& ip, std::atomic<int>& counter, const int total, const bool quiet, int timeoutMs) {
        const bool res = pingFallback(ip, quiet, timeoutMs);

        {
            std::lock_guard<std::mutex> lock(outputMutex);

            if (res && !quiet) {
                std::cerr << ip << std::endl;
            }

            const int done = ++counter;
            constexpr int width = 30;
            const int filled = static_cast<int>((static_cast<double>(done) * width) / total);

            if (!quiet) {
                std::cerr << "\r[";
                for (int i = 0; i < width; ++i) std::cerr << (i < filled ? '#' : '.');
                std::cerr << "] " << done << "/" << total << std::flush;
            }
        }

        return res;
    }

    bool ping(const std::string& ip, bool quiet, int timeoutMs) {
        std::atomic<int> dummy = 0;
        return ping(ip, dummy, 1, quiet, timeoutMs);
    }
}
